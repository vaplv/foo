#ifndef RDR_FONT_C_H
#define RDR_FONT_C_H

#include "renderer/rdr_error.h"
#include <stddef.h>
#include <wchar.h>

struct rdr_glyph {
  size_t width;
  struct {
    float x;
    float y;
  } tex[2], pos[2];
};

struct rb_tex2d;
struct rdr_font;

extern enum rdr_error
rdr_get_font_glyph
  (struct rdr_font* font,
   wchar_t character,
   struct rdr_glyph* desc);

extern enum rdr_error
rdr_get_font_texture
  (struct rdr_font* font,
   struct rb_tex2d** tex);

#endif /* RDR_FONT_C_H */

