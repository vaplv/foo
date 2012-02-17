#ifndef RB_OGL3_BUFFER_H
#define RB_OGL3_BUFFER_H

#include "render_backend/rb_types.h"
#include "sys/ref_count.h"
#include <GL/gl.h>

struct rb_context;

struct rb_buffer {
  struct ref ref;
  struct rb_context* ctxt;
  GLuint name;
  GLenum target;
  GLenum usage;
  GLsizei size;
  enum rb_buffer_target binding;
};

#endif /* RB_OGL3_BUFFER_H */

