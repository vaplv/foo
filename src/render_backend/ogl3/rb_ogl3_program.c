#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/ogl3/rb_ogl3_context.h"
#include "render_backend/ogl3/rb_ogl3_program.h"
#include "render_backend/ogl3/rb_ogl3_shader.h"
#include "render_backend/rb.h"
#include "sys/list.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_program(struct ref* ref)
{
  struct list_node* node = NULL;
  struct rb_context* ctxt = NULL;
  struct rb_program* prog = NULL;
  assert(ref);

  prog = CONTAINER_OF(ref, struct rb_program, ref);
  ctxt = prog->ctxt;

  if(ctxt->state_cache.current_program == prog->name)
    RB(bind_program(ctxt, NULL));

  LIST_FOR_EACH(node, &prog->attached_shader_list) {
    struct rb_shader* shader = CONTAINER_OF(node, struct rb_shader, attachment);
    RB(detach_shader(prog, shader));
  }
  if(prog->name != 0)
    OGL(DeleteProgram(prog->name));
  if(prog->log)
    MEM_FREE(ctxt->allocator, prog->log);
  MEM_FREE(ctxt->allocator, prog);
  RB(context_ref_put(ctxt));
}

/*******************************************************************************
 *
 * Program functions.
 *
 ******************************************************************************/
EXPORT_SYM int
rb_create_program(struct rb_context* ctxt, struct rb_program** out_program)
{
  int err = 0;
  struct rb_program* program = NULL;

  if(!ctxt || !out_program)
    goto error;

  program = MEM_CALLOC(ctxt->allocator, 1, sizeof(struct rb_program));
  if(!program)
    goto error;
  ref_init(&program->ref);
  list_init(&program->attached_shader_list);
  RB(context_ref_get(ctxt));
  program->ctxt = ctxt;

  program->name = OGL(CreateProgram());
  if(program->name == 0)
    goto error;

exit:
  if(out_program)
    *out_program = program;
  return err;

error:
  if(program) {
    RB(program_ref_put(program));
    program = NULL;
  }
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_program_ref_get(struct rb_program* program)
{
  if(!program)
    return -1;
  ref_get(&program->ref);
  return 0;
}

EXPORT_SYM int
rb_program_ref_put(struct rb_program* program)
{
  if(!program)
    return -1;
  ref_put(&program->ref, release_program);
  return 0;
}

EXPORT_SYM int
rb_attach_shader(struct rb_program* program, struct rb_shader* shader)
{
  if(!program || !shader || !is_list_empty(&shader->attachment))
    return -1;

  OGL(AttachShader(program->name, shader->name));
  list_add(&program->attached_shader_list, &shader->attachment);
  return 0;
}

EXPORT_SYM int
rb_detach_shader(struct rb_program* program, struct rb_shader* shader)
{
  if(!program || !shader || is_list_empty(&shader->attachment))
    return -1;

  #ifndef NDEBUG
  {
    struct list_node* node = NULL;
    int found = 0;
    LIST_FOR_EACH(node, &program->attached_shader_list) {
      if(node == &shader->attachment) {
        found = 1;
        break; 
      }
    }
    if(!found)
      return -1;
  }
  #endif
  OGL(DetachShader(program->name, shader->name));
  list_del(&shader->attachment);
  return 0;
}

EXPORT_SYM int
rb_link_program(struct rb_program* program)
{
  int err = 0;
  GLint status = GL_TRUE;

  if(!program)
    goto error;

  OGL(LinkProgram(program->name));
  OGL(GetProgramiv(program->name, GL_LINK_STATUS, &status));
  program->is_linked = (status == GL_TRUE);

  if(!program->is_linked) {
    int log_length = 0;

    if(program->ctxt->state_cache.current_program == program->name)
      rb_bind_program(program->ctxt, NULL);

    OGL(GetProgramiv(program->name, GL_INFO_LOG_LENGTH, &log_length));

    program->log = MEM_REALLOC
      (program->ctxt->allocator, program->log, log_length*sizeof(char));
    if(!program->log)
      goto error;

    OGL(GetProgramInfoLog(program->name, log_length, NULL, program->log));
    err = -1;
  } else {
    MEM_FREE(program->ctxt->allocator, program->log);
    program->log = NULL;
  }

exit:
  return err;

error:
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_get_program_log(struct rb_program* program, const char** out_log)
{
  if(!program || !out_log)
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

  ctxt->state_cache.current_program = program ? program->name : 0;
  OGL(UseProgram(ctxt->state_cache.current_program));
  return 0;
}

