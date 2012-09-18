#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/ogl3/rb_ogl3_context.h"
#include "render_backend/ogl3/rb_ogl3_program.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct rb_uniform {
  struct ref ref;
  struct rb_context* ctxt;
  GLint location;
  GLuint index;
  GLenum type;
  struct rb_program* program;
  char* name;
  void (*set)(GLint location, int nb, const void* data);
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
#define UNIFORM_VALUE(suffix)\
  static void\
  uniform_##suffix(GLint location, int nb, const void* data)\
  {\
    OGL(Uniform##suffix(location, nb, data));\
  }\

#define UNIFORM_MATRIX_VALUE(suffix)\
  static void\
  uniform_matrix_##suffix(GLint location, int nb, const void* data)\
  {\
    OGL(UniformMatrix##suffix(location, nb, GL_FALSE, data));\
  }\

UNIFORM_VALUE(1fv)
UNIFORM_VALUE(2fv)
UNIFORM_VALUE(3fv)
UNIFORM_VALUE(4fv)
UNIFORM_VALUE(1iv)
UNIFORM_VALUE(1uiv)
UNIFORM_MATRIX_VALUE(2fv)
UNIFORM_MATRIX_VALUE(3fv)
UNIFORM_MATRIX_VALUE(4fv)

static void
(*get_uniform_setter(GLenum uniform_type))(GLint, int, const void*)
{
  switch(uniform_type) {
    case GL_FLOAT: return &uniform_1fv; break;
    case GL_FLOAT_VEC2: return &uniform_2fv; break;
    case GL_FLOAT_VEC3: return &uniform_3fv; break;
    case GL_FLOAT_VEC4: return &uniform_4fv; break;
    case GL_FLOAT_MAT2: return &uniform_matrix_2fv; break;
    case GL_FLOAT_MAT3: return &uniform_matrix_3fv; break;
    case GL_FLOAT_MAT4: return &uniform_matrix_4fv; break;
    case GL_UNSIGNED_INT: return &uniform_1uiv; break;
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_1D:
    case GL_UNSIGNED_INT_SAMPLER_2D:
      return &uniform_1iv;
      break;
    default:
      assert(0);
      return NULL;
      break;
  }
}

static int
get_active_uniform
  (struct rb_context* ctxt,
   struct rb_program* program,
   GLuint index,
   GLsizei bufsize,
   GLchar* buffer,
   struct rb_uniform** out_uniform)
{
  struct rb_uniform* uniform = NULL;
  GLsizei uniform_namelen = 0;
  GLint uniform_size = 0;
  GLenum uniform_type;
  int err = 0;

  if(!ctxt
  || !program
  || !out_uniform
  || bufsize < 0
  || (bufsize && !buffer))
    goto error;

  OGL(GetActiveUniform
      (program->name,
       index,
       bufsize,
       &uniform_namelen,
       &uniform_size,
       &uniform_type,
       buffer));

  uniform = MEM_CALLOC(ctxt->allocator, 1, sizeof(struct rb_uniform));
  if(!uniform)
    goto error;
  ref_init(&uniform->ref);
  RB(context_ref_get(ctxt));
  uniform->ctxt = ctxt;
  RB(program_ref_get(program));
  uniform->program = program;
  uniform->index = index;
  uniform->type = uniform_type;
  uniform->set = get_uniform_setter(uniform_type);

  if(buffer) {
    /* Add 1 to namelen <=> include the null character. */
    ++uniform_namelen;

    uniform->name = MEM_ALLOC
      (ctxt->allocator, sizeof(char) * uniform_namelen);
    if(!uniform->name)
      goto error;

    uniform->name = strncpy(uniform->name, buffer, uniform_namelen);
    uniform->location = OGL(GetUniformLocation(program->name, uniform->name));
  }

exit:
  *out_uniform = uniform;
  return err;

error:
  if(uniform) {
    RB(uniform_ref_put(uniform));
    uniform = NULL;
  }
  err = -1;
  goto exit;
}

static void
release_uniform(struct ref* ref)
{
  struct rb_context* ctxt = NULL;
  struct rb_uniform* uniform = NULL;
  assert(ref);

  uniform = CONTAINER_OF(ref, struct rb_uniform, ref);
  ctxt = uniform->ctxt;

  if(uniform->program)
    RB(program_ref_put(uniform->program));
  if(uniform->name)
    MEM_FREE(ctxt->allocator, uniform->name);
  MEM_FREE(ctxt->allocator, uniform);
  RB(context_ref_put(ctxt));
}

/*******************************************************************************
 *
 * Uniform implementation.
 *
 ******************************************************************************/
int
rb_get_named_uniform
  (struct rb_context* ctxt,
   struct rb_program* program,
   const char* name,
   struct rb_uniform** out_uniform)
{
  struct rb_uniform* uniform = NULL;
  GLuint uniform_index = GL_INVALID_INDEX;
  size_t name_len = 0;
  int err = 0;

  if(!ctxt || !program || !name || !out_uniform)
    goto error;

  if(!program->is_linked)
    goto error;

  OGL(GetUniformIndices(program->name, 1, &name, &uniform_index));
  if(uniform_index == GL_INVALID_INDEX)
    goto error;

  err = get_active_uniform(ctxt, program, uniform_index, 0, NULL, &uniform);
  if(err != 0)
    goto error;

  name_len = strlen(name) + 1;
  uniform->name = MEM_ALLOC(ctxt->allocator, sizeof(char) * name_len);
  if(!uniform->name)
    goto error;

  uniform->name = strncpy(uniform->name, name, name_len);
  uniform->location = OGL(GetUniformLocation(program->name, uniform->name));

exit:
  *out_uniform = uniform;
  return err;

error:
  if(uniform) {
    RB(uniform_ref_put(uniform));
    uniform = NULL;
  }
  err = -1;
  goto exit;
}

int
rb_get_uniforms
  (struct rb_context* ctxt,
   struct rb_program* prog,
   size_t* out_nb_uniforms,
   struct rb_uniform* dst_uniform_list[])
{
  GLchar* uniform_buffer = NULL;
  int uniform_buflen = 0;
  int nb_uniforms = 0;
  int uniform_id = 0;
  int err = 0;

  if(!ctxt || !prog || !out_nb_uniforms)
    goto error;

  if(!prog->is_linked)
    goto error;

  OGL(GetProgramiv(prog->name, GL_ACTIVE_UNIFORMS, &nb_uniforms));
  assert(nb_uniforms >= 0);

  if(dst_uniform_list) {
      OGL(GetProgramiv
        (prog->name, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniform_buflen));
    uniform_buffer = MEM_ALLOC
      (ctxt->allocator, sizeof(GLchar) * uniform_buflen);
    if(!uniform_buffer)
      goto error;

    for(uniform_id = 0; uniform_id < nb_uniforms; ++uniform_id) {
      struct rb_uniform* uniform = NULL;

      err = get_active_uniform
        (ctxt, prog, uniform_id, uniform_buflen, uniform_buffer, &uniform);
      if(err != 0)
        goto error;

      dst_uniform_list[uniform_id] = uniform;
    }
  }

exit:
  if(uniform_buffer)
    MEM_FREE(ctxt->allocator, uniform_buffer);
  if(out_nb_uniforms)
    *out_nb_uniforms = nb_uniforms;
  return err;

error:
  if(dst_uniform_list) {
    /* NOTE: uniform_id <=> nb uniforms in dst_uniform_list; */
    int i = 0;
    for(i = 0; i < uniform_id; ++i) {
      RB(uniform_ref_put(dst_uniform_list[i]));
      dst_uniform_list[i] = NULL;
    }
  }
  nb_uniforms = 0;
  err = -1;
  goto exit;
}

int
rb_uniform_ref_get(struct rb_uniform* uniform)
{
  if(!uniform)
    return -1;
  ref_get(&uniform->ref);
  return 0;
}

int
rb_uniform_ref_put(struct rb_uniform* uniform)
{
  if(!uniform)
    return -1;
  ref_put(&uniform->ref, release_uniform);
  return 0;
}

int
rb_uniform_data(struct rb_uniform* uniform, int nb, const void* data)
{
  if(!uniform || !data)
    return -1;
  if(nb <= 0)
    return -1;

  assert(uniform->set != NULL);
  OGL(UseProgram(uniform->program->name));
  uniform->set(uniform->location, nb, data);
  OGL(UseProgram(uniform->ctxt->state_cache.current_program));
  return 0;
}

int
rb_get_uniform_desc(struct rb_uniform* uniform, struct rb_uniform_desc* desc)
{
  if(!uniform || !desc)
    return -1;
  desc->name = uniform->name;
  desc->type = ogl3_to_rb_type(uniform->type);
  return 0;
}

