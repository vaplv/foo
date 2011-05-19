#include "render_backend/ogl3/rb_ogl3.h"
#include "sys/sys.h"
#include <stdlib.h>
#include <assert.h>

#define UNIFORM_VALUE(suffix)\
  static void\
  uniform_##suffix(GLint location, int nb, void* data)\
  {\
    OGL(Uniform##suffix(location, nb, data));\
  }\

#define UNIFORM_MATRIX_VALUE(suffix)\
  static void\
  uniform_matrix_##suffix(GLint location, int nb, void* data)\
  {\
    OGL(UniformMatrix##suffix(location, nb, GL_FALSE, data));\
  }\

UNIFORM_VALUE(1fv);
UNIFORM_VALUE(2fv);
UNIFORM_VALUE(3fv);
UNIFORM_VALUE(4fv);
UNIFORM_VALUE(1iv);
UNIFORM_MATRIX_VALUE(2fv);
UNIFORM_MATRIX_VALUE(3fv);
UNIFORM_MATRIX_VALUE(4fv);

static void
(*get_uniform_setter(GLenum uniform_type))(GLint, int, void*)
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

EXPORT_SYM int
rb_get_uniform
  (struct rb_context* ctxt,
   struct rb_program* program,
   const char* name,
   struct rb_uniform** out_uniform)
{
  int err = 0;
  GLint uniform_location = -1;
  GLenum uniform_type = GL_FLOAT;
  GLuint uniform_index = GL_INVALID_INDEX;
  void (*uniform_setter)(GLint, int, void*) = NULL;
  struct rb_uniform* uniform = NULL;

  if(!ctxt || !program || !name || !out_uniform)
    goto error;

  if(!program->is_linked)
    goto error;

  uniform_location = OGL(GetUniformLocation(program->name, name));
  if(uniform_location == -1)
    goto error;

  OGL(GetUniformIndices(program->name, 1, &name, &uniform_index));
  if(uniform_index == GL_INVALID_INDEX)
    goto error;

  OGL(GetActiveUniformsiv
      (program->name, 1, &uniform_index, GL_UNIFORM_TYPE, &uniform_type));
  uniform_setter = get_uniform_setter(uniform_type);
  if(!uniform_setter)
    goto error;

  uniform = malloc(sizeof(struct rb_uniform));
  if(!uniform)
    goto error;

  uniform->location = uniform_location;
  uniform->index = uniform_index;
  uniform->program_name = program->name;
  uniform->set = uniform_setter;

  *out_uniform = uniform;

exit:
  return err;

error:
  err = -1;
  free(uniform);

  goto exit;
}

EXPORT_SYM int
rb_release_uniform(struct rb_context* ctxt, struct rb_uniform* uniform)
{
  if(!ctxt || !uniform)
    return -1;

  free(uniform);
  return 0;
}

EXPORT_SYM int
rb_uniform_data
  (struct rb_context* ctxt, struct rb_uniform* uniform, int nb, void* data)
{
  if(!ctxt || !uniform || !data)
    return -1;

  if(nb <= 0)
    return -1;

  assert(uniform->set != NULL);
  OGL(UseProgram(uniform->program_name));
  uniform->set(uniform->location, nb, data);
  OGL(UseProgram(ctxt->current_program));
  return 0;
}

