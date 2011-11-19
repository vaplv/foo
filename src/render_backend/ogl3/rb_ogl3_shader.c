#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <stdlib.h>

static const GLenum rb_to_ogl3_shader_type[] = {
  [RB_VERTEX_SHADER] = GL_VERTEX_SHADER,
  [RB_GEOMETRY_SHADER] = GL_GEOMETRY_SHADER,
  [RB_FRAGMENT_SHADER] = GL_FRAGMENT_SHADER
};

EXPORT_SYM int
rb_create_shader
  (struct rb_context* ctxt,
   enum rb_shader_type type,
   const char* source,
   int length,
   struct rb_shader** out_shader)
{
  int err = 0;
  struct rb_shader* shader = NULL;

  if(!ctxt || !out_shader)
    goto error;

  shader = MEM_ALLOC_I(ctxt->allocator, sizeof(struct rb_shader));
  if(!shader)
    goto error;

  shader->log = NULL;
  shader->is_attached = 0;
  shader->type = rb_to_ogl3_shader_type[type];
  shader->name = OGL(CreateShader(shader->type));
  if(shader->name == 0)
    goto error;

  err = rb_shader_source(ctxt, shader, source, length);
  *out_shader = shader;

exit:
  return err;

error:
  err = -1;
  if(shader->name != 0)
    OGL(DeleteShader(shader->name));
  if(shader) {
    MEM_FREE_I(ctxt->allocator, shader->log);
    MEM_FREE_I(ctxt->allocator, shader);
  }

  goto exit;
}

EXPORT_SYM int
rb_shader_source
  (struct rb_context* ctxt,
   struct rb_shader* shader,
   const char* source,
   int length)
{
  int err = 0;
  GLint status = GL_TRUE;

  if(!ctxt || !shader || (length > 0 && !source))
    goto error;

  OGL(ShaderSource(shader->name, 1, (const char**)&source, &length));
  OGL(CompileShader(shader->name));
  OGL(GetShaderiv(shader->name, GL_COMPILE_STATUS, &status));

  if(status == GL_FALSE) {
    int log_length = 0;
    OGL(GetShaderiv(shader->name, GL_INFO_LOG_LENGTH, &log_length));

    shader->log = MEM_REALLOC_I
      (ctxt->allocator, shader->log, log_length*sizeof(char));
    if(!shader->log)
      goto error;

    OGL(GetShaderInfoLog(shader->name, log_length, NULL, shader->log));
    err = -1;
  } else {
    MEM_FREE_I(ctxt->allocator, shader->log);
    shader->log = NULL;
  }

exit:
  return err;

error:
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_free_shader(struct rb_context* ctxt, struct rb_shader* shader)
{
  if(!ctxt || !shader)
    return -1;

  OGL(DeleteShader(shader->name));
  MEM_FREE_I(ctxt->allocator, shader->log);
  MEM_FREE_I(ctxt->allocator, shader);
  return 0;
}

EXPORT_SYM int
rb_get_shader_log
  (struct rb_context* ctxt,
   struct rb_shader* shader,
   const char** out_log)
{
  if(!ctxt || !shader || !out_log)
    return -1;

  *out_log = shader->log;
  return 0;
}

EXPORT_SYM int
rb_is_shader_attached
  (struct rb_context* ctxt,
   struct rb_shader* shader,
   int* out_is_attached)
{
  if(!ctxt || !shader || !out_is_attached)
    return -1;

  *out_is_attached = shader->is_attached;
  return 0;
}

