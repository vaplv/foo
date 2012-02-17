#ifndef RB_OGL3_CONTEXT_H
#define RB_OGL3_CONTEXT_H

#include "sys/ref_count.h"
#include <GL/gl.h>

struct mem_allocator;

struct rb_context {
  struct ref ref;
  struct mem_allocator* allocator;
  GLuint texture_binding_2d;
  GLuint buffer_binding[2];
  GLuint vertex_array_binding;
  GLuint current_program;
};

#endif /* RB_OGL3_CONTEXT_H */

