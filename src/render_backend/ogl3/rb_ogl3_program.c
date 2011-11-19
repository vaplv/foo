#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <stdlib.h>
#include <string.h>

EXPORT_SYM int
rb_create_program(struct rb_context* ctxt, struct rb_program** out_program)
{
  int err = 0;
  struct rb_program* program = NULL;

  if(!ctxt || !out_program)
    goto error;

  program = MEM_CALLOC_I(ctxt->allocator, 1, sizeof(struct rb_program));
  if(!program)
    goto error;

  program->name = OGL(CreateProgram());
  if(program->name == 0)
    goto error;

  *out_program = program;

exit:
  return err;

error:
  err = -1;
  if(program->name != 0)
    OGL(DeleteProgram(program->name));
  MEM_FREE_I(ctxt->allocator, program);
  goto exit;
}

EXPORT_SYM int
rb_free_program(struct rb_context* ctxt, struct rb_program* program)
{
  if(!ctxt || !program)
    return -1;

  if(program->ref_count > 0)
    return -1;

  OGL(DeleteProgram(program->name));
  MEM_FREE_I(ctxt->allocator, program->log);
  MEM_FREE_I(ctxt->allocator, program);
  return 0;
}

EXPORT_SYM int
rb_attach_shader
  (struct rb_context* ctxt,
   struct rb_program* program,
   struct rb_shader* shader)
{
  if(!ctxt || !program || !shader || shader->is_attached)
    return -1;

  OGL(AttachShader(program->name, shader->name));
  shader->is_attached = 1;
  return 0;
}

EXPORT_SYM int
rb_detach_shader
  (struct rb_context* ctxt,
   struct rb_program* program,
   struct rb_shader* shader)
{
  if(!ctxt || !program || !shader || !shader->is_attached)
    return -1;

  OGL(DetachShader(program->name, shader->name));
  shader->is_attached = 0;
  return 0;
}

EXPORT_SYM int
rb_link_program(struct rb_context* ctxt, struct rb_program* program)
{
  int err = 0;
  GLint status = GL_TRUE;

  if(!ctxt || !program)
    goto error;

  if(program->ref_count > 0)
    goto error;

  OGL(LinkProgram(program->name));
  OGL(GetProgramiv(program->name, GL_LINK_STATUS, &status));
  program->is_linked = (status == GL_TRUE);

  if(!program->is_linked) {
    int log_length = 0;

    if(ctxt->current_program == program->name)
      rb_bind_program(ctxt, NULL);

    OGL(GetProgramiv(program->name, GL_INFO_LOG_LENGTH, &log_length));

    program->log = MEM_REALLOC_I
      (ctxt->allocator, program->log, log_length*sizeof(char));
    if(!program->log)
      goto error;

    OGL(GetProgramInfoLog(program->name, log_length, NULL, program->log));
    err = -1;
  } else {
    MEM_FREE_I(ctxt->allocator, program->log);
    program->log = NULL;
  }

exit:
  return err;

error:
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_get_program_log
  (struct rb_context* ctxt, struct rb_program* program, const char** out_log)
{
  if(!ctxt || !program || !out_log)
    return -1;

  *out_log = program->log;
  return 0;
}

EXPORT_SYM int
rb_bind_program(struct rb_context* ctxt, struct rb_program* program)
{
  if(!ctxt)
    return -1;

  if(program && !program->is_linked)
    return -1;

  ctxt->current_program = program ? program->name : 0;
  OGL(UseProgram(ctxt->current_program));
  return 0;
}

