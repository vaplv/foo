#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

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
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
      return &uniform_1iv;
      break;
    default:
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

  uniform = MEM_CALLOC_I(ctxt->allocator, 1, sizeof(struct rb_uniform));
  if(!uniform)
    goto error;

  uniform->index = index;
  uniform->program = program;
  uniform->program->ref_count += 1;
  uniform->type = uniform_type;
  uniform->set = get_uniform_setter(uniform_type);

  if(buffer) {
    /* Add 1 to namelen <=> include the null character. */
    ++uniform_namelen;

    uniform->name = MEM_ALLOC_I
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
  if(uniform)
    rb_release_uniforms(ctxt, 1, &uniform);

  uniform = NULL;
  err = -1;
  goto exit;
}

/*******************************************************************************
 *
 * Uniform implementation.
 *
 ******************************************************************************/
EXPORT_SYM int
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
  uniform->name = MEM_ALLOC_I(ctxt->allocator, sizeof(char) * name_len);
  if(!uniform->name)
    goto error;

  uniform->name = strncpy(uniform->name, name, name_len);
  uniform->location = OGL(GetUniformLocation(program->name, uniform->name));

exit:
  *out_uniform = uniform;
  return err;

error:
  if(uniform)
    rb_release_uniforms(ctxt, 1, &uniform);

  uniform = NULL;
  err = -1;
  goto exit;
}

EXPORT_SYM int
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
    uniform_buffer = MEM_ALLOC_I
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
    MEM_FREE_I(ctxt->allocator, uniform_buffer);
  if(out_nb_uniforms)
    *out_nb_uniforms = nb_uniforms;
  return err;

error:
  if(dst_uniform_list) {
    /* NOTE: attr_id <=> nb attribs in dst_attrib_list; */
    rb_release_uniforms(ctxt, uniform_id, dst_uniform_list);
  }
  nb_uniforms = 0;
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_release_uniforms
  (struct rb_context* ctxt,
   size_t nb_uniforms,
   struct rb_uniform* uniform_list[])
{
  size_t i = 0;
  int err = 0;

  if(!ctxt || (nb_uniforms && !uniform_list))
    goto error;

  for(i = 0; i < nb_uniforms; ++i) {
    struct rb_uniform* uniform = uniform_list[i];
    if(!uniform) {
      err = -1;
    } else {
      assert(uniform->name);
      assert(uniform->program->ref_count > 0);

      uniform->program->ref_count -= 1;
      MEM_FREE_I(ctxt->allocator, uniform->name);
      MEM_FREE_I(ctxt->allocator, uniform);
    }
  }

exit:
  return err;

error:
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_uniform_data
  (struct rb_context* ctxt,
   struct rb_uniform* uniform,
   int nb,
   const void* data)
{
  if(!ctxt || !uniform || !data)
    return -1;

  if(nb <= 0)
    return -1;

  assert(uniform->set != NULL);
  OGL(UseProgram(uniform->program->name));
  uniform->set(uniform->location, nb, data);
  OGL(UseProgram(ctxt->current_program));
  return 0;
}

EXPORT_SYM int
rb_get_uniform_desc
  (struct rb_context* ctxt,
   struct rb_uniform* uniform,
   struct rb_uniform_desc* out_desc)
{
  if(!ctxt || !uniform || !out_desc)
    return -1;

  out_desc->name = uniform->name;
  out_desc->type = ogl3_to_rb_type(uniform->type);
  return 0;
}

