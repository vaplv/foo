#ifndef RB_OGL3_CONTEXT_H
#define RB_OGL3_CONTEXT_H

#include "render_backend/ogl3/rb_ogl3.h"
#include "sys/ref_count.h"
#include <GL/gl.h>

struct mem_allocator;

struct rb_context {
  struct ref ref;
  struct mem_allocator* allocator;
  struct rb_config config;
  /* Basic state cache. */
  struct state_cache {
    GLuint buffer_binding[RB_OGL3_NB_BUFFER_TARGETS];
    GLuint current_program;
    GLuint framebuffer_binding;
    GLuint sampler_binding[RB_OGL3_MAX_TEXTURE_UNITS];
    GLuint texture_binding_2d[RB_OGL3_MAX_TEXTURE_UNITS];
    GLuint vertex_array_binding;
    GLenum active_texture;
  } state_cache;
};

#endif /* RB_OGL3_CONTEXT_H */

