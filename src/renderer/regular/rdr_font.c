#include "render_backend/rbi.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_font.h"
#include "renderer/rdr_system.h"
#include "stdlib/sl.h"
#include "stdlib/sl_hash_table.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#define GLYPH_BORDER 1

struct rdr_font {
  struct ref ref;
  struct rdr_system* sys;
  struct sl_hash_table* glyph_htbl;
  struct rb_tex2d* cache_tex;
  struct {
    size_t Bpp;
    size_t width;
    size_t height;
    unsigned char* buffer;
  } cache_img;
  size_t line_space;
};

/*******************************************************************************
 *
 * Glyph hash table.
 *
 ******************************************************************************/
struct pair {
  wchar_t character;
  struct {
    size_t width;
    float u[2];
    float v[2];
  } glyph;
};

static const void*
get_key(const void* pair)
{
  return (const void*)(&((const struct pair*)pair)->character);
}

static int
cmp_key(const void* p0, const void* p1)
{
  return *(const wchar_t*)p0 - *(const wchar_t*)p1;
}

/*******************************************************************************
 *
 * Helper functions and data structure.
 *
 ******************************************************************************/
struct node {
  struct node* left;
  struct node* right;
  size_t x, y;
  size_t width, height;
  size_t id;
};

#define IS_LEAF(n) (!((n)->left || (n)->right))

static struct node*
insert_rect
  (struct mem_allocator* allocator,
   struct node* node,
   size_t width,
   size_t height)
{
  struct node* ret_node = NULL;

  if(!node) {
    ret_node = NULL;
  } else if(!IS_LEAF(node)) {
    ret_node = insert_rect(allocator, node->left, width, height);
    if(!ret_node && node->right)
      ret_node = insert_rect(allocator, node->right, width, height);
  } else {
    /* Adjust the width and height in order to take care of the glyph border. */
    width += GLYPH_BORDER;
    height += GLYPH_BORDER;

    if(width > node->width || height > node->height) {
      /* The leaf is too small to store the rectangle. */
      ret_node = NULL;
    } else {
      const size_t w = node->width - width;
      const size_t h = node->height - height;

      node->left = MEM_CALLOC(allocator, 1, sizeof(struct node));
      assert(node->left);
      node->right = MEM_CALLOC(allocator, 1, sizeof(struct node));
      assert(node->right);

      if(w <= h) {
        /* +-----+
         * |R |  | ##: current node
         * +--+L | L : left node
         * |##|  | R : right node
         * +--+--+
         */
        node->left->x = node->x + width;
        node->left->y = node->y;
        node->left->width = w;
        node->left->height = height;

        node->right->x = node->x;
        node->right->y = node->y + height;
        node->right->width = node->width;
        node->right->height = h;
      } else {
        /* +-------+
         * |   L   | ##: current node
         * +--+----+ L : left node
         * |##| R  | R : right node
         * +--+----+
         */
        node->left->x = node->x;
        node->left->y = node->y + height;
        node->left->width = width;
        node->left->height = h;

        node->right->x = node->x + width;
        node->right->y = node->y;
        node->right->width = w;
        node->right->height = node->height;
      }
      node->width = width;
      node->height = height;
      ret_node = node;
    }
  }
  return ret_node;
}
static void
extend_width(struct node* node, size_t current_width, size_t size)
{
  assert(node);
  if(!IS_LEAF(node)) {
    extend_width(node->left, current_width, size);
    extend_width(node->right, current_width, size);
  } else {
    if(node->x + node->width == current_width)
      node->width += size;
  }
}

static void
extend_height(struct node* node, size_t current_height, size_t size)
{
  assert(node);
  if(!IS_LEAF(node)) {
    extend_height(node->left, current_height, size);
    extend_height(node->right, current_height, size);
  } else {
    if(node->y + node->height == current_height)
      node->height += size;
  }
}

static void
free_binary_tree(struct mem_allocator* allocator, struct node* node)
{
  if(node->left)
    free_binary_tree(allocator, node->left);
  if(node->right)
    free_binary_tree(allocator, node->right);
  MEM_FREE(allocator, node);
}

static void
copy_bitmap
  (unsigned char* restrict dst,
   size_t dst_pitch,
   const unsigned char* restrict src,
   size_t src_pitch,
   size_t width,
   size_t height,
   size_t Bpp)
{
  size_t i = 0;

  assert(dst && dst_pitch && src && src_pitch && width && height && Bpp);
  assert(!IS_MEMORY_OVERLAPPED(dst, height*dst_pitch, src, height*src_pitch));

  for(i = 0; i < height; ++i) {
    unsigned char* dst_row = dst + i * dst_pitch;
    const unsigned char* src_row = src + i * src_pitch;
    memcpy(dst_row, src_row, width * Bpp);
  }
}

static void
compute_initial_cache_size
  (size_t nb_glyphs,
   const struct rdr_glyph_desc* glyph_list,
   size_t* out_width,
   size_t* out_height)
{
  size_t width = 0;
  size_t height = 0;
  size_t i = 0;

  assert(glyph_list && out_width && out_height);

  for(i = 0; i < nb_glyphs; ++i) {
    const size_t w =  glyph_list[i].bitmap.width;
    const size_t h =  glyph_list[i].bitmap.height;
    width = w > width ? w : width;
    height = h > height ? h : height;
  }
  /* We multiply the max size required by a glyph by 4 in each dimension in
   * order to store at least 16 glyphs in a the texture. */
  *out_width = (width + GLYPH_BORDER) * 4;
  *out_height = (height + GLYPH_BORDER) * 4;
}

static void
fill_font_cache
  (const struct node* node,
   struct rdr_font* font,
   const struct rdr_glyph_desc* glyph_list)
{
  if(!IS_LEAF(node)) {
    struct pair* pair = NULL;
    const struct rdr_glyph_desc* glyph_desc = glyph_list + node->id;
    const size_t cache_Bpp = font->cache_img.Bpp;
    const size_t cache_pitch = font->cache_img.width * cache_Bpp;
    const size_t rcp_cache_width = 1.f / (float)font->cache_img.width;
    const size_t rcp_cache_height = 1.f / (float)font->cache_img.height;
    unsigned char* dst = NULL;

    SL(hash_table_find(font->glyph_htbl, &glyph_desc->character,(void**)&pair));
    assert(pair);
    pair->glyph.width = glyph_desc->width;
    pair->glyph.u[0] = (float)node->x * rcp_cache_width;
    pair->glyph.v[0] = (float)node->y * rcp_cache_height;
    pair->glyph.u[1] = (float)(node->x + node->width) * rcp_cache_width;
    pair->glyph.v[1] = (float)(node->y + node->height) * rcp_cache_height;

    assert(glyph_desc->bitmap.bytes_per_pixel == cache_Bpp);
    dst =
      font->cache_img.buffer
    + (node->y == 0 ? GLYPH_BORDER : node->y) * cache_pitch
    + (node->x == 0 ? GLYPH_BORDER : node->x) * cache_Bpp;
    copy_bitmap
      (dst,
       cache_pitch,
       glyph_desc->bitmap.buffer,
       glyph_desc->bitmap.width * cache_Bpp,
       glyph_desc->bitmap.width,
       glyph_desc->bitmap.height,
       cache_Bpp);

    fill_font_cache(node->left, font, glyph_list);
    fill_font_cache(node->right, font, glyph_list);
  }
}

static int
cmp_glyph(const void* a, const void* b)
{
  const struct rdr_glyph_desc* glyph0 = a;
  const struct rdr_glyph_desc* glyph1 = b;
  const size_t area0 = glyph0->bitmap.width * glyph0->bitmap.height;
  const size_t area1 = glyph1->bitmap.width * glyph1->bitmap.height;
  return (int)(area1 - area0);
}

static enum rb_tex_format
Bpp_to_rb_tex_format(size_t Bpp)
{
  enum rb_tex_format tex_format = RB_R;
  switch(Bpp) {
    case 1:
      tex_format = RB_R;
      break;
    case 3:
      tex_format = RB_RGB;
      break;
    default:
      assert(false);
      break;
  }
  return tex_format;
}

static void
reset_font(struct rdr_font* font)
{
  int rb_err = 0;
  assert(font);

  SL(hash_table_clear(font->glyph_htbl));

  rb_err = font->sys->rb.tex2d_data
    (font->sys->ctxt,
     font->cache_tex,
     (struct rb_tex2d_desc[]){{0, 0, 0, RB_R, 0}},
     NULL);
  assert(rb_err == 0);

  if(font->cache_img.buffer)
    MEM_FREE(font->sys->allocator, font->cache_img.buffer);
  memset(&font->cache_img, 0, sizeof(font->cache_img));

  font->line_space = 0;
}

static void
release_font(struct ref* ref)
{
  struct rdr_font* font = NULL;
  struct rdr_system* sys = NULL;
  assert(NULL != ref);

  font = CONTAINER_OF(ref, struct rdr_font, ref);

  if(font->glyph_htbl)
    SL(free_hash_table(font->glyph_htbl));

  if(font->cache_tex) {
    int err = 0;
    err = font->sys->rb.free_tex2d(font->sys->ctxt, font->cache_tex);
    assert(0 == err);
  }
  if(font->cache_img.buffer)
    MEM_FREE(font->sys->allocator, font->cache_img.buffer);
  sys = font->sys;
  MEM_FREE(font->sys->allocator, font);
  RDR(system_ref_put(sys));
}

#undef IS_LEAF

/*******************************************************************************
 *
 * Font functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rdr_error
rdr_create_font(struct rdr_system* sys, struct rdr_font** out_font)
{
  struct rdr_font* font = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  int rb_err = 0;

  if(!sys || !out_font) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  font = MEM_CALLOC(sys->allocator, 1, sizeof(struct rdr_font));
  if(!font) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  ref_init(&font->ref);
  font->sys = sys;
  RDR(system_ref_get(sys));

  sl_err = sl_create_hash_table
    (sizeof(struct pair),
     ALIGNOF(struct pair),
     sizeof(wchar_t),
     cmp_key,
     get_key,
     font->sys->allocator,
     &font->glyph_htbl);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

  rb_err = sys->rb.create_tex2d(sys->ctxt, &font->cache_tex);
  assert(rb_err == 0);

exit:
  if(out_font)
    *out_font = font;
  return rdr_err;
error:
  if(font) {
    RDR(font_ref_put(font));
    font = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_font_ref_get(struct rdr_font* font)
{
  if(!font)
    return RDR_INVALID_ARGUMENT;
  ref_get(&font->ref);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_font_ref_put(struct rdr_font* font)
{
  if(!font)
    return RDR_INVALID_ARGUMENT;
  ref_put(&font->ref, release_font);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_font_data
  (struct rdr_font* font,
   size_t line_space,
   size_t nb_glyphs,
   const struct rdr_glyph_desc* glyph_list)
{
  struct rb_tex2d_desc tex2d_desc;
  struct rdr_glyph_desc* sorted_glyphs = NULL;
  struct node* root = NULL;
  size_t Bpp = 0;
  size_t i = 0;
  size_t cache_width = 0;
  size_t cache_height = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int rb_err = 0;
  memset(&tex2d_desc, 0, sizeof(tex2d_desc));

  if(!font || (nb_glyphs && !glyph_list)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  reset_font(font);
  font->line_space = line_space;

  if(0 == nb_glyphs)
    goto exit;

  /* Sort the input glyphs in descending order with respect to their
   * bitmap size. */
  sorted_glyphs = MEM_CALLOC
    (font->sys->allocator, nb_glyphs, sizeof(struct rdr_glyph_desc));
  if(!sorted_glyphs) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  memcpy(sorted_glyphs, glyph_list, sizeof(struct rdr_glyph_desc)*nb_glyphs);
  qsort(sorted_glyphs, nb_glyphs, sizeof(struct rdr_glyph_desc), cmp_glyph);

  /* Create the binary tree data structure used to pack the glyphs into the
   * cache texture. */
  Bpp = sorted_glyphs[i].bitmap.bytes_per_pixel;
  if(Bpp != 1 && Bpp != 3) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  compute_initial_cache_size
    (nb_glyphs, sorted_glyphs, &cache_width, &cache_height);
  root = MEM_CALLOC(font->sys->allocator, 1, sizeof(struct node));
  if(!root) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  root->x = 0;
  root->y = 0;
  root->width = cache_width;
  root->height = cache_height;
  root->id = SIZE_MAX;

  for(i = 0; i < nb_glyphs; ++i) {
    void* data = NULL;
    struct node* node = NULL;
    size_t width = 0;
    size_t height = 0;

    /* Check the conformity of the glyph bitmap format. */
    if(sorted_glyphs[i].bitmap.bytes_per_pixel != Bpp) {
      rdr_err = RDR_INVALID_ARGUMENT;
      goto error;
    }
    width = sorted_glyphs[i].bitmap.width;
    height = sorted_glyphs[i].bitmap.height;
    /* Check whether the glyph character is already registered or not. */
    SL(hash_table_find(font->glyph_htbl, &sorted_glyphs[i].character, &data));
    if(data != NULL) {
      continue;
    } else {
      struct pair pair;
      enum sl_error sl_err = SL_NO_ERROR;

      memset(&pair, 0, sizeof(pair));
      pair.character = sorted_glyphs[i].character;
      sl_err = sl_hash_table_insert(font->glyph_htbl, (void*)&pair);
      if(sl_err != SL_NO_ERROR) {
        rdr_err = sl_to_rdr_error(sl_err);
        goto error;
      }
    }
    /* Pack the glyph bitmap. */
    node = insert_rect(font->sys->allocator, root, width, height);
    while(!node) {
      const size_t max_tex_size = font->sys->cfg.max_tex_size;
      const size_t extend_x = width / 2;
      const size_t extend_y = height / 2;
      const bool can_extend_w = (cache_width + extend_x) <= max_tex_size;
      const bool can_extend_h = (cache_height + extend_y) <= max_tex_size;
      const bool extend_w = can_extend_w && cache_width < cache_height;
      const bool extend_h = can_extend_h && !extend_w;

      if(extend_w) {
        extend_width(root, cache_width, extend_x);
        cache_width += extend_x;
      } else if(extend_h) {
        extend_height(root, cache_height, extend_y);
        cache_height += extend_y;
      } else if(can_extend_w) {
        extend_width(root, cache_width, extend_x);
        cache_width += extend_x;
      } else if(can_extend_h) {
        extend_height(root, cache_height, extend_y);
        cache_height += extend_y;
      } else {
        rdr_err = RDR_MEMORY_ERROR;
        goto error;
      }
      node = insert_rect(font->sys->allocator, root, width, height);
    }
    if(!node) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }
    node->id = i;
  }
  /* Use the pack information to fill the font glyph cache. */
  font->cache_img.Bpp = Bpp;
  font->cache_img.width = cache_width;
  font->cache_img.height = cache_height;
  font->cache_img.buffer = MEM_CALLOC
    (font->sys->allocator, cache_width * cache_height, Bpp);
  if(!font->cache_img.buffer) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  fill_font_cache(root, font, sorted_glyphs);
  /* Setup the cache texture. */
  tex2d_desc.width = cache_width;
  tex2d_desc.height = cache_height;
  tex2d_desc.level = 0;
  tex2d_desc.format = Bpp_to_rb_tex_format(Bpp);
  tex2d_desc.compress = 0;
  rb_err = font->sys->rb.tex2d_data
    (font->sys->ctxt,
     font->cache_tex,
     &tex2d_desc,
     (void*)font->cache_img.buffer);
  assert(0 == rb_err);

exit:
  if(root)
    free_binary_tree(font->sys->allocator, root);
  if(sorted_glyphs)
    MEM_FREE(font->sys->allocator, sorted_glyphs);
  return rdr_err;
error:
  if(font)
    reset_font(font);
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_font_bitmap_cache
  (const struct rdr_font* font,
   size_t* width,
   size_t* height,
   size_t* bytes_per_pixel,
   const unsigned char** bitmap_cache)
{
  if(!font)
    return RDR_INVALID_ARGUMENT;

  if(width)
    *width = font->cache_img.width;
  if(height)
    *height = font->cache_img.height;
  if(bytes_per_pixel)
    *bytes_per_pixel = font->cache_img.Bpp;
  if(bitmap_cache)
    *bitmap_cache = font->cache_img.buffer;

  return RDR_NO_ERROR;
}

#undef GLYPH_BORDER

