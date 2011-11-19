#ifndef RB_OGL3_H
#define RB_OGL3_H

#include "render_backend/rb_types.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <assert.h>

#ifndef NDEBUG
  #include <GL/glu.h>
  #include <stdio.h>
  #define OGL(func)\
    gl##func;\
    {\
      GLenum gl_error = glGetError();\
      if(gl_error != GL_NO_ERROR) {\
        fprintf(stderr, "error:opengl: %s\n", gluErrorString(gl_error));\
        assert(gl_error == GL_NO_ERROR);\
      }\
    }
#else
  #define OGL(func) gl##func
#endif

struct rb_context {
  struct mem_allocator* allocator;
  GLuint texture_binding_2d;
  GLuint buffer_binding[2];
  GLuint vertex_array_binding;
  GLuint current_program;
};

struct rb_tex2d {
  GLuint name;
};

struct rb_buffer {
  GLuint name;
  GLenum target;
  GLenum usage;
  GLsizei size;
  enum rb_buffer_target binding;
};

struct rb_vertex_array {
  GLuint name;
};

struct rb_shader {
  GLuint name;
  GLenum type;
  int is_attached;
  char* log;
};

struct rb_program {
  GLuint name;
  int is_linked;
  int ref_count;
  char* log;
};

struct rb_uniform {
  GLint location;
  GLuint index;
  GLenum type;
  struct rb_program* program;
  char* name;
  void (*set)(GLint location, int nb, const void* data);
};

struct rb_attrib {
  GLuint index;
  GLenum type;
  struct rb_program* program;
  char* name;
  void (*set)(GLuint, const void* data);
};

static inline enum rb_type
ogl3_to_rb_type(GLenum attrib_type)
{
  switch(attrib_type) {
    case GL_FLOAT: return RB_FLOAT;
    case GL_FLOAT_VEC2: return RB_FLOAT2;
    case GL_FLOAT_VEC3: return RB_FLOAT3;
    case GL_FLOAT_VEC4: return RB_FLOAT4;
    case GL_FLOAT_MAT4: return RB_FLOAT4x4;
    default: return RB_UNKNOWN_TYPE;
  }
}

#endif /* RB_OGL3_H */

