#ifndef RB_OGL3_TEXTURE_H
#define RB_OGL3_TEXTURE_H

#include "render_backend/ogl3/rb_ogl3.h"
#include "sys/ref_count.h"

struct rb_context;
struct rb_buffer;
struct mip_level {
  size_t pixbuf_offset;
  unsigned int width;
  unsigned int height;
};

struct rb_tex2d {
  struct ref ref;
  struct rb_context* ctxt;
  struct rb_buffer* pixbuf;
  struct mip_level* mip_list;
  unsigned int mip_count;
  GLenum format;
  GLenum internal_format;
  GLenum type;
  GLuint name;
};

extern size_t
rb_ogl3_sizeof_pixel
  (GLenum format, 
   GLenum type);

extern int
rb_ogl3_is_uint_type
  (GLenum type);

extern int
rb_ogl3_is_int_type
  (GLenum type);

#endif /* RB_OGL3_TEXTURE_H */
