#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/ogl3/rb_ogl3_context.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>

struct rb_tex2d {
  struct ref ref;
  struct rb_context* ctxt;
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
    case RB_R: ogl3_ifmt = GL_RED; break;
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

static void
release_tex2d(struct ref* ref)
{
  struct rb_context* ctxt = NULL;
  struct rb_tex2d* tex = NULL;
  assert(ref);

  tex = CONTAINER_OF(ref, struct rb_tex2d, ref);
  ctxt = tex->ctxt;

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
rb_create_tex2d(struct rb_context* ctxt, struct rb_tex2d** out_tex)
{
  struct rb_tex2d* tex = NULL;

  if(!ctxt || !out_tex)
    return -1;

  tex = MEM_ALLOC(ctxt->allocator, sizeof(struct rb_tex2d));
  if(!tex)
    return -1;
  ref_init(&tex->ref);
  RB(context_ref_get(ctxt));
  tex->ctxt = ctxt;

  OGL(GenTextures(1, &tex->name));
  *out_tex = tex;
  return 0;
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
rb_bind_tex2d(struct rb_context* ctxt, struct rb_tex2d* tex)
{
  if(!ctxt)
    return -1;

  ctxt->texture_binding_2d = tex ? tex->name : 0;
  OGL(BindTexture(GL_TEXTURE_2D, ctxt->texture_binding_2d));
  return 0;
}

EXPORT_SYM int
rb_tex2d_data
  (struct rb_tex2d* tex,
   const struct rb_tex2d_desc* desc,
   void* data)
{
  if(!tex || !desc)
    return -1;

  if(desc->width < 0 || desc->height < 0 || desc->level < 0)
    return -1;

  GLenum gl_format = ogl3_format(desc->format);
  GLenum gl_internal_format =
    desc->compress 
    ? ogl3_compressed_internal_format(desc->format)
    : ogl3_internal_format(desc->format);

  OGL(BindTexture(GL_TEXTURE_2D, tex->name));
  OGL(TexImage2D
      (GL_TEXTURE_2D,
       desc->level,
       gl_internal_format,
       desc->width,
       desc->height,
       0,
       gl_format,
       GL_UNSIGNED_BYTE,
       data));
  OGL(BindTexture(GL_TEXTURE_2D, tex->ctxt->texture_binding_2d));

  return 0;
}

