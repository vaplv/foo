#include "render_backend/rbi.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_font_c.h"
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
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_CHAR ((wchar_t)~0)

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
  size_t min_glyph_width;
  int min_glyph_pos_y;
};

/*******************************************************************************
 *
 * Glyph hash table.
 *
 ******************************************************************************/
struct pair {
  wchar_t character;
  struct rdr_glyph glyph;
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
#define EXTENDABLE_X 1
#define EXTENDABLE_Y 2

struct node {
  struct node* left;
  struct node* right;
  size_t x, y;
  size_t width, height;
  int extendable_flag;
  size_t id;
};

#define GLYPH_BORDER 1
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

      if(w > h) {
        /* +-----+
         * |R |  | ##: current node
         * +--+L | L : left node
         * |##|  | R : right node
         * +--+--+
         */
        node->left->x = node->x + width;
        node->left->y = node->y;
        node->left->width = w;
        node->left->height = node->height;
        node->left->extendable_flag = node->extendable_flag;

        node->right->x = node->x;
        node->right->y = node->y + height;
        node->right->width = width;
        node->right->height = h;
        node->right->extendable_flag = node->extendable_flag & (~EXTENDABLE_X);
     } else {
        /* +-------+
         * |   L   | ##: current node
         * +--+----+ L : left node
         * |##| R  | R : right node
         * +--+----+
         */
        node->left->x = node->x;
        node->left->y = node->y + height;
        node->left->width = node->width;
        node->left->height = h;
        node->left->extendable_flag = node->extendable_flag;

        node->right->x = node->x + width;
        node->right->y = node->y;
        node->right->width = w;
        node->right->height = height;
        node->right->extendable_flag = node->extendable_flag & (~EXTENDABLE_Y);
      }
      node->width = width;
      node->height = height;
      node->extendable_flag = 0;
      ret_node = node;
    }
  }
  return ret_node;
}
static void
extend_width(struct node* node, size_t size)
{
  assert(node);
  if(!IS_LEAF(node)) {
    extend_width(node->left, size);
    extend_width(node->right, size);
  } else {
    if((node->extendable_flag & EXTENDABLE_X) != 0)
      node->width += size;
  }
}

static void
extend_height(struct node* node, size_t size)
{
  assert(node);
  if(!IS_LEAF(node)) {
    extend_height(node->left, size);
    extend_height(node->right, size);
  } else {
    if((node->extendable_flag & EXTENDABLE_Y) != 0)
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
  assert(node && font);
  if(!IS_LEAF(node)) {
    struct pair* pair = NULL;
    unsigned char* dst = NULL;
    const struct rdr_glyph_desc* glyph_desc = glyph_list + node->id;
    const size_t cache_Bpp = font->cache_img.Bpp;
    const size_t cache_pitch = font->cache_img.width * cache_Bpp;
    const float rcp_cache_width = 1.f / (float)font->cache_img.width;
    const float rcp_cache_height = 1.f / (float)font->cache_img.height;
    const size_t w = node->width - GLYPH_BORDER;
    const size_t h = node->height - GLYPH_BORDER;
    const size_t x = node->x == 0 ? GLYPH_BORDER : node->x;
    const size_t y = node->y == 0 ? GLYPH_BORDER : node->y;
    const size_t glyph_bmp_size = 
      glyph_desc->bitmap.width
    * glyph_desc->bitmap.height
    * glyph_desc->bitmap.bytes_per_pixel;

    SL(hash_table_find(font->glyph_htbl, &glyph_desc->character,(void**)&pair));
    assert(pair);

    pair->glyph.width = glyph_desc->width;
    pair->glyph.tex[0].x = (float)x * rcp_cache_width;
    pair->glyph.tex[0].y = (float)(y + h) * rcp_cache_height;
    pair->glyph.tex[1].x = (float)(x + w) * rcp_cache_width;
    pair->glyph.tex[1].y = (float)y * rcp_cache_height;

    pair->glyph.pos[0].x = (float)glyph_desc->bitmap_left;
    pair->glyph.pos[0].y = (float)glyph_desc->bitmap_top;
    pair->glyph.pos[1].x = (float)(glyph_desc->bitmap_left + w); 
    pair->glyph.pos[1].y = (float)(glyph_desc->bitmap_top + h);

    /* The glyph bitmap size may be equal to zero (e.g.: the space char). */
    if(0 != glyph_bmp_size) {
      assert(glyph_desc->bitmap.bytes_per_pixel == cache_Bpp);
      dst = font->cache_img.buffer + y * cache_pitch + x * cache_Bpp;
      copy_bitmap
        (dst,
         cache_pitch,
         glyph_desc->bitmap.buffer,
         glyph_desc->bitmap.width * cache_Bpp,
         glyph_desc->bitmap.width,
         glyph_desc->bitmap.height,
         cache_Bpp);
    }
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
  assert(font);

  SL(hash_table_clear(font->glyph_htbl));

  if(font->cache_tex) {
    RBI(font->sys->rb, tex2d_ref_put(font->cache_tex));
    font->cache_tex = NULL;
  }

  if(font->cache_img.buffer)
    MEM_FREE(font->sys->allocator, font->cache_img.buffer);
  memset(&font->cache_img, 0, sizeof(font->cache_img));

  font->line_space = 0;
}

static enum rdr_error
create_default_glyph
  (struct mem_allocator* allocator,
   size_t width, 
   size_t height,
   size_t Bpp,
   struct rdr_glyph_desc* glyph)
{
  unsigned char* buffer = NULL;
  const size_t size = width * height * Bpp;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(glyph && allocator);
  glyph->character = DEFAULT_CHAR;
  glyph->width = width;
  glyph->bitmap_left = 0;
  glyph->bitmap_top = 0;
  glyph->bitmap.width = width;
  glyph->bitmap.height = height;
  glyph->bitmap.bytes_per_pixel = Bpp;
  if(size) {
    buffer = MEM_CALLOC(allocator, 1, size);
    if(NULL == buffer) {
      rdr_err =  RDR_MEMORY_ERROR;
      goto error;
    } else {
      const size_t pitch = width * Bpp;
      size_t y = 0;
      memset(buffer, 255, pitch);
      memset(buffer + (height - 1) * pitch, 255, pitch);
      for(y = 1; y < height - 1; ++y) {
        memset(buffer + y * pitch, 255, Bpp);
        memset(buffer + y * pitch + (width - 1) * Bpp, 255, Bpp);
      }
    }
  }
exit:
  if(glyph)
    glyph->bitmap.buffer = buffer;
  return rdr_err;
error: 
  if(buffer)
    MEM_FREE(allocator, buffer);
  goto exit;
}

static void
free_default_glyph
  (struct mem_allocator* allocator, 
   struct rdr_glyph_desc* glyph)
{
  assert(allocator && glyph);
  if(glyph->bitmap.buffer)
    MEM_FREE(allocator, glyph->bitmap.buffer);
  memset(glyph, 0, sizeof(struct rdr_glyph_desc));
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
  if(font->cache_tex)
    RBI(font->sys->rb, tex2d_ref_put(font->cache_tex));
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
  struct rdr_glyph_desc default_glyph; 
  struct rdr_glyph_desc* sorted_glyphs = NULL;
  struct node* root = NULL;
  size_t Bpp = 0;
  size_t cache_width = 0;
  size_t cache_height = 0;
  size_t i = 0;
  size_t max_bmp_width = 0;
  size_t max_bmp_height = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  memset(&tex2d_desc, 0, sizeof(tex2d_desc));
  memset(&default_glyph, 0, sizeof(default_glyph));

  if(!font || (nb_glyphs && !glyph_list)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  if(0 == nb_glyphs)
    goto exit;

  reset_font(font);

  /* Retrieve global font metrics. */
  font->line_space = line_space;
  font->min_glyph_width = SIZE_MAX;
  font->min_glyph_pos_y = INT_MAX;
  for(i = 0; i < nb_glyphs; ++i) {
    font->min_glyph_width = MIN(font->min_glyph_width, glyph_list[i].width);
    font->min_glyph_pos_y = 
      MIN(font->min_glyph_pos_y, glyph_list[i].bitmap_left);
    max_bmp_width = MAX(max_bmp_width, glyph_list[i].bitmap.width);
    max_bmp_height = MAX(max_bmp_height, glyph_list[i].bitmap.height);
  }
  Bpp = glyph_list[0].bitmap.bytes_per_pixel;
  if(Bpp != 1 && Bpp != 3) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  rdr_err = create_default_glyph
    (font->sys->allocator, max_bmp_width, max_bmp_height, Bpp, &default_glyph);
  if(RDR_NO_ERROR != rdr_err)
    goto error;

  /* Sort the input glyphs in descending order with respect to their
   * bitmap size. */
  sorted_glyphs = MEM_CALLOC /* +1 <=> default glyph. */
    (font->sys->allocator, nb_glyphs + 1, sizeof(struct rdr_glyph_desc));
  if(!sorted_glyphs) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  memcpy(sorted_glyphs, &default_glyph, sizeof(struct rdr_glyph_desc));
  memcpy(sorted_glyphs + 1, glyph_list, sizeof(struct rdr_glyph_desc)*nb_glyphs);
  ++nb_glyphs; /* Take into account the default glyph. */

  qsort(sorted_glyphs, nb_glyphs, sizeof(struct rdr_glyph_desc), cmp_glyph);

  /* Create the binary tree data structure used to pack the glyphs into the
   * cache texture. */
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
  root->extendable_flag = EXTENDABLE_X | EXTENDABLE_Y;
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
      const size_t extend_x = MAX(width / 2, 1);
      const size_t extend_y = MAX(height / 2, 1);
      const bool can_extend_w = (cache_width + extend_x) <= max_tex_size;
      const bool can_extend_h = (cache_height + extend_y) <= max_tex_size;
      const bool extend_w = can_extend_w && cache_width < cache_height;
      const bool extend_h = can_extend_h && !extend_w;

      if(extend_w) {
        extend_width(root, extend_x);
        cache_width += extend_x;
      } else if(extend_h) {
        extend_height(root, extend_y);
        cache_height += extend_y;
      } else if(can_extend_w) {
        extend_width(root, extend_x);
        cache_width += extend_x;
      } else if(can_extend_h) {
        extend_height(root, extend_y);
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
  tex2d_desc.mip_count = 1;
  tex2d_desc.format = Bpp_to_rb_tex_format(Bpp);
  tex2d_desc.usage = RB_USAGE_IMMUTABLE;
  tex2d_desc.compress = 0;
  RBI(font->sys->rb, create_tex2d
    (font->sys->ctxt, 
     &tex2d_desc, 
     (const void**)&font->cache_img.buffer, 
     &font->cache_tex));

exit:
  if(font)
    free_default_glyph(font->sys->allocator, &default_glyph);
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
rdr_get_font_metrics(struct rdr_font* font, struct rdr_font_metrics* metrics)
{
  if(!font || !metrics)
    return RDR_INVALID_ARGUMENT;
  metrics->line_space = font->line_space;
  metrics->min_glyph_width = font->min_glyph_width;
  metrics->min_glyph_pos_y = font->min_glyph_pos_y;
  return RDR_NO_ERROR;
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

/*******************************************************************************
 *
 * Private font functions.
 *
 ******************************************************************************/
enum rdr_error
rdr_get_font_glyph
  (struct rdr_font* font,
   wchar_t character,
   struct rdr_glyph* glyph)
{
  struct pair* pair = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!font || !glyph) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  SL(hash_table_find(font->glyph_htbl, &character,(void**)&pair));

  if(pair == NULL) {
    SL(hash_table_find
      (font->glyph_htbl, (wchar_t[]){DEFAULT_CHAR}, (void**)&pair));
    assert(NULL != pair);
  }
  memcpy(glyph, &pair->glyph, sizeof(struct rdr_glyph));

exit:
  return rdr_err;
error:
  goto exit;
}

enum rdr_error
rdr_get_font_texture(struct rdr_font* font, struct rb_tex2d** tex)
{
  if(!font || !tex)
    return RDR_INVALID_ARGUMENT;
  *tex = font->cache_tex;
  return RDR_NO_ERROR;
}

#undef GLYPH_BORDER

