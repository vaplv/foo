#ifndef RB_OGL3_SHADER_H
#define RB_OGL3_SHADER_H

#include "sys/ref_count.h"
#include <GL/gl.h>

struct rb_context;

struct rb_shader {
  struct ref ref;
  struct rb_context* ctxt;
  GLuint name;
  GLenum type;
  int is_attached;
  char* log;
};

#endif  /* RB_OGL3_SHADER_H */

