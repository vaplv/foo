#ifndef RB_OGL3_CONTEXT_H
#define RB_OGL3_CONTEXT_H

#include "render_backend/ogl3/rb_ogl3.h"
#include "sys/ref_count.h"
#include <GL/gl.h>

struct mem_allocator;

struct rb_context {
  struct ref ref;
  struct mem_allocator* allocator;
  GLuint current_program;
  GLuint buffer_binding[2];
  GLuint sampler_binding[RB_OGL3_MAX_TEXTURE_UNITS];
  GLuint texture_binding_2d;
  GLuint vertex_array_binding;
};

#endif /* RB_OGL3_CONTEXT_H */

