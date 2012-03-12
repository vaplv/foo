#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/ogl3/rb_ogl3_buffers.h"
#include "render_backend/ogl3/rb_ogl3_context.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

struct mip_level {
  size_t pixbuf_offset;
  unsigned int width;
  unsigned int height;
};

struct rb_tex2d {
  struct ref ref;
  struct rb_context* ctxt;
  struct rb_buffer* pixbuf;
  struct mip_level* mip_list;
  unsigned int mip_count;
  GLenum format;
  GLenum internal_format;
  GLuint name;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static FINLINE GLenum
ogl3_compressed_internal_format(enum rb_tex_format fmt)
{
  GLenum ogl3_ifmt = GL_NONE;
  switch(fmt) {
    case RB_R: ogl3_ifmt = GL_COMPRESSED_RED; break;
    case RB_RGB: ogl3_ifmt = GL_COMPRESSED_RGB; break;
    case RB_RGBA: ogl3_ifmt = GL_COMPRESSED_RGBA; break;
    case RB_SRGB: ogl3_ifmt = GL_COMPRESSED_SRGB; break;
    case RB_SRGBA: ogl3_ifmt = GL_COMPRESSED_SRGB_ALPHA; break;
    default:
      assert(0);
      break;
  }
  return ogl3_ifmt;
}

static FINLINE GLenum
ogl3_internal_format(enum rb_tex_format fmt)
{
  GLenum ogl3_ifmt = GL_NONE;
  switch(fmt) {
    case RB_R: ogl3_ifmt = GL_R8; break;
    case RB_RGB: ogl3_ifmt = GL_RGB8; break;
    case RB_RGBA: ogl3_ifmt = GL_RGBA8; break;
    case RB_SRGB: ogl3_ifmt = GL_SRGB8; break;
    case RB_SRGBA: ogl3_ifmt = GL_SRGB8_ALPHA8; break;
    default:
      assert(0);
      break;
  }
  return ogl3_ifmt;
}

static FINLINE GLenum
ogl3_format(enum rb_tex_format fmt)
{
  GLenum ogl3_fmt = GL_NONE;
  switch(fmt) {
    case RB_R: ogl3_fmt = GL_RED; break;
    case RB_RGB: ogl3_fmt = GL_RGB; break;
    case RB_RGBA: ogl3_fmt = GL_RGBA; break;
    case RB_SRGB: ogl3_fmt = GL_RGB; break;
    case RB_SRGBA: ogl3_fmt = GL_RGBA; break;
    default:
      assert(0);
      break;
  }
  return ogl3_fmt;
}

static FINLINE size_t
sizeof_ogl3_format(GLenum fmt)
{
  size_t size = 0;
  switch(fmt) {
    case GL_RED:
      size = 1 * sizeof(GLbyte);
      break;
    case GL_RGB:
      size = 3 * sizeof(GLbyte);
      break;
    case GL_RGBA:
      size = 4 * sizeof(GLbyte);
      break;
    default:
      assert(0);
      break;
  }
  return size;
}

static void
release_tex2d(struct ref* ref)
{
  struct rb_context* ctxt = NULL;
  struct rb_tex2d* tex = NULL;
  size_t i = 0;
  assert(ref);

  tex = CONTAINER_OF(ref, struct rb_tex2d, ref);
  ctxt = tex->ctxt;

  for(i = 0; i < RB_OGL3_MAX_TEXTURE_UNITS; ++i) {
    if(ctxt->state_cache.texture_binding_2d[i] == tex->name)
      RB(bind_tex2d(ctxt, NULL, i));
  }

  if(tex->mip_list)
    MEM_FREE(ctxt->allocator, tex->mip_list);
  if(tex->pixbuf)
    RB(buffer_ref_put(tex->pixbuf));
  OGL(DeleteTextures(1, &tex->name));
  MEM_FREE(ctxt->allocator, tex);
  RB(context_ref_put(ctxt));
}

/*******************************************************************************
 *
 * Texture 2D functions.
 *
 ******************************************************************************/
EXPORT_SYM int
rb_create_tex2d
  (struct rb_context* ctxt,
   const struct rb_tex2d_desc* desc,
   const void* init_data[],
   struct rb_tex2d** out_tex)
{
  struct rb_ogl3_buffer_desc buffer_desc;
  struct rb_tex2d* tex = NULL;
  size_t pixel_size = 0;
  size_t size = 0;
  unsigned int i = 0;
  int err = 0;

  if(!ctxt 
  || !desc 
  || !init_data 
  || !out_tex 
  || !desc->mip_count)
    goto error;

  tex = MEM_ALLOC(ctxt->allocator, sizeof(struct rb_tex2d));
  if(!tex)
    goto error;
  ref_init(&tex->ref);
  RB(context_ref_get(ctxt));
  tex->ctxt = ctxt;
  OGL(GenTextures(1, &tex->name));

  OGL(BindTexture(GL_TEXTURE_2D, tex->name));
  OGL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, desc->mip_count - 1));
  OGL(BindTexture
    (GL_TEXTURE_2D, 
     ctxt->state_cache.texture_binding_2d[ctxt->state_cache.active_texture]));

  tex->mip_count = desc->mip_count;
  tex->mip_list =  MEM_CALLOC
    (ctxt->allocator, tex->mip_count, sizeof(struct mip_level));
  if(NULL == tex->mip_list)
    goto error;

  tex->format = ogl3_format(desc->format);
  tex->internal_format =
    desc->compress
    ? ogl3_compressed_internal_format(desc->format)
    : ogl3_internal_format(desc->format);

  /* Define the mip parameters and the required size into the pixel buffer. */
  pixel_size = sizeof_ogl3_format(tex->format);
  for(i = 0, size = 0; i < desc->mip_count; ++i) {
    tex->mip_list[i].pixbuf_offset = size;
    tex->mip_list[i].width = MAX(desc->width / (1<<i), 1);
    tex->mip_list[i].height = MAX(desc->height / (1<<i), 1);
    size += tex->mip_list[i].width * tex->mip_list[i].height * pixel_size;
  }

  /* Create the pixel buffer. */
  buffer_desc.usage = RB_USAGE_DEFAULT;
  buffer_desc.target = RB_OGL3_BIND_PIXEL_DOWNLOAD_BUFFER;
  buffer_desc.size = size;
  err = rb_ogl3_create_buffer(ctxt, &buffer_desc, NULL, &tex->pixbuf);
  if(0 != err)
    goto error;

  /* Setup the texture data. */
  for(i = 0; i < desc->mip_count; ++i) {
    if(init_data[i])
      RB(tex2d_data(tex, i, init_data[i]));
  }

exit:
  if(out_tex)
    *out_tex = tex;
  return err;
error:
  if(tex) {
    RB(tex2d_ref_put(tex));
    tex = NULL;
  }
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_tex2d_ref_get(struct rb_tex2d* tex)
{
  if(!tex)
    return -1;
  ref_get(&tex->ref);
  return 0;
}

EXPORT_SYM int
rb_tex2d_ref_put(struct rb_tex2d* tex)
{
  if(!tex)
    return -1;
  ref_put(&tex->ref, release_tex2d);
  return 0;
}

EXPORT_SYM int
rb_bind_tex2d
  (struct rb_context* ctxt,
   struct rb_tex2d* tex,
   unsigned int tex_unit)
{
  if(!ctxt || tex_unit > RB_OGL3_MAX_TEXTURE_UNITS)
    return -1;

  if(tex_unit != ctxt->state_cache.active_texture) {
    ctxt->state_cache.active_texture = GL_TEXTURE0 + tex_unit;
    OGL(ActiveTexture(ctxt->state_cache.active_texture));
  }

  ctxt->state_cache.texture_binding_2d[tex_unit] = tex ? tex->name : 0;
  OGL(BindTexture
    (GL_TEXTURE_2D, ctxt->state_cache.texture_binding_2d[tex_unit]));
  return 0;
}

EXPORT_SYM int
rb_tex2d_data(struct rb_tex2d* tex, unsigned int level, const void* data)
{
  struct state_cache* state_cache = NULL;
  struct mip_level* mip_level = NULL;
  size_t pixel_size = 0;
  size_t mip_size = 0;

  if(!tex || level > tex->mip_count)
    return -1;
  state_cache = &tex->ctxt->state_cache;
  mip_level = tex->mip_list + level;
  pixel_size = sizeof_ogl3_format(tex->format);
  mip_size = mip_level->width * mip_level->height * pixel_size;

  RB(buffer_data(tex->pixbuf, mip_level->pixbuf_offset, mip_size, data));

  OGL(BindBuffer(tex->pixbuf->target, tex->pixbuf->name));
  OGL(BindTexture(GL_TEXTURE_2D, tex->name));

  #define TEX_IMAGE_2D \
    OGL(TexImage2D \
      (GL_TEXTURE_2D, \
       level, \
       tex->internal_format, \
       mip_level->width, \
       mip_level->height, \
       0, \
       tex->format, \
       GL_UNSIGNED_BYTE, \
       BUFFER_OFFSET(mip_level->pixbuf_offset)))

  /* We assume that the default pixel storage alignment is set to 4. */
  if(pixel_size == 4) {
    TEX_IMAGE_2D;
  }  else {
    OGL(PixelStorei(GL_UNPACK_ALIGNMENT, 1));
    TEX_IMAGE_2D;
    OGL(PixelStorei(GL_UNPACK_ALIGNMENT, 4));
  }

  #undef TEX_IMAGE_2D

  OGL(BindTexture
    (GL_TEXTURE_2D,
     state_cache->texture_binding_2d[state_cache->active_texture]));
  OGL(BindBuffer
    (tex->pixbuf->target,
     state_cache->buffer_binding[tex->pixbuf->binding]));

  return 0;
}

#undef BUFFER_OFFSET

