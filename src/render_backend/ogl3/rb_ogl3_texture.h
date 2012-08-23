#ifndef RB_OGL3_TEXTURE_H
#define RB_OGL3_TEXTURE_H

#include "render_backend/ogl3/rb_ogl3.h"
#include "sys/ref_count.h"

struct rb_context;
struct rb_buffer;
struct mip_level;

struct rb_tex2d {
  struct ref ref;
  struct rb_context* ctxt;
  struct rb_buffer* pixbuf;
  struct mip_level* mip_list;
  unsigned int mip_count;
  GLenum format;
  GLenum internal_format;
  GLuint name;
};

#endif /* RB_OGL3_TEXTURE_H */
