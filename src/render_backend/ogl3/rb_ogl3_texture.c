#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <stdlib.h>

static const GLenum ogl3_compressed_internal_format[] = {
  [RB_R] = GL_COMPRESSED_RED,
  [RB_RGB] = GL_COMPRESSED_RGB,
  [RB_RGBA] = GL_COMPRESSED_RGBA,
  [RB_SRGB] = GL_COMPRESSED_SRGB,
  [RB_SRGBA] = GL_COMPRESSED_SRGB_ALPHA,
};

static const GLenum ogl3_internal_format[] = {
  [RB_R] = GL_RED,
  [RB_RGB] = GL_RGB8,
  [RB_RGBA] = GL_RGBA8,
  [RB_SRGB] = GL_SRGB8,
  [RB_SRGBA] = GL_SRGB8_ALPHA8
};

static const GLenum ogl3_format[] = {
  [RB_R] = GL_RED,
  [RB_RGB] = GL_RGB,
  [RB_RGBA] = GL_RGBA,
  [RB_SRGB] = GL_RGB,
  [RB_SRGBA] = GL_RGBA
};

EXPORT_SYM int
rb_create_tex2d(struct rb_context* ctxt, struct rb_tex2d** out_tex)
{
  struct rb_tex2d* tex = NULL;

  if(!ctxt || !out_tex)
    return -1;

  tex = MEM_ALLOC(ctxt->allocator, sizeof(struct rb_tex2d));
  if(!tex)
    return -1;

  OGL(GenTextures(1, &tex->name));
  *out_tex = tex;
  return 0;
}

EXPORT_SYM int
rb_free_tex2d(struct rb_context* ctxt, struct rb_tex2d* tex)
{
  if(!ctxt || !tex)
    return -1;

  OGL(DeleteTextures(1, &tex->name));
  MEM_FREE(ctxt->allocator, tex);
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
  (struct rb_context* ctxt,
   struct rb_tex2d* tex,
   const struct rb_tex2d_desc* desc,
   void* data)
{
  if(!ctxt || !tex || !desc)
    return -1;

  if(desc->width < 0 || desc->height < 0 || desc->level < 0)
    return -1;

  GLenum gl_format = ogl3_format[desc->format];
  GLenum gl_internal_format =
    desc->compress ? ogl3_compressed_internal_format[desc->format]
                   : ogl3_internal_format[desc->format];

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
  OGL(BindTexture(GL_TEXTURE_2D, ctxt->texture_binding_2d));

  return 0;
}

