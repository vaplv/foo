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
  int bind_id;
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
  char* log;
};

struct rb_uniform {
  GLint location;
  GLuint index;
  GLuint program_name;
  void (*set)(GLint location, int nb, void* data);
};

#endif /* RB_OGL3_H */

