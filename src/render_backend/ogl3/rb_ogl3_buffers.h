#ifndef RB_OGL3_BUFFERS_H
#define RB_OGL3_BUFFERS_H

#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/rb_types.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <GL/gl.h>

struct rb_context;

struct rb_buffer {
  struct ref ref;
  struct rb_context* ctxt;
  GLuint name;
  GLenum target;
  GLenum usage;
  GLsizei size;
  enum rb_ogl3_buffer_target binding; /* used to indexed the state cache. */
};

struct rb_ogl3_buffer_desc {
  size_t size;
  enum rb_ogl3_buffer_target target;
  enum rb_usage usage;
};

LOCAL_SYM int
rb_ogl3_create_buffer
  (struct rb_context* ctxt, 
   const struct rb_ogl3_buffer_desc* desc, 
   const void* init_data, 
   struct rb_buffer** buffer);

LOCAL_SYM int
rb_ogl3_bind_buffer
  (struct rb_context* ctxt,
   struct rb_buffer* buffer, 
   enum rb_ogl3_buffer_target target);

#endif /* RB_OGL3_BUFFERS_H */

