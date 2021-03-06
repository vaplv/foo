#ifndef RDR_FONT_H
#define RDR_FONT_H

#include "renderer/rdr.h"
#include "renderer/rdr_error.h"
#include <stddef.h>

struct rdr_font;
struct rdr_glyph_desc {
   wchar_t character;
   size_t width;
   int bitmap_left;
   int bitmap_top;
   struct {
     size_t width;
     size_t height;
     size_t bytes_per_pixel;
     unsigned char* buffer;
   } bitmap;
};
struct rdr_font_metrics {
  size_t line_space;
  size_t min_glyph_width;
  int min_glyph_pos_y;
};
struct rdr_system;

RDR_API enum rdr_error
rdr_create_font
  (struct rdr_system* sys,
   struct rdr_font** font);

RDR_API enum rdr_error
rdr_font_ref_get
  (struct rdr_font* font);

RDR_API enum rdr_error
rdr_font_ref_put
  (struct rdr_font* font);

RDR_API enum rdr_error
rdr_font_data
  (struct rdr_font* font,
   size_t line_space,
   size_t nb_glyphs,
   const struct rdr_glyph_desc* glyph_list);

RDR_API enum rdr_error
rdr_get_font_metrics
  (struct rdr_font* font,
   struct rdr_font_metrics* metrics);

RDR_API enum rdr_error
rdr_font_bitmap_cache
  (const struct rdr_font* font,
   size_t* width, /* May be NULL. */
   size_t* height, /* May be NULL. */
   size_t* bytest_per_pixel, /* May be NULL. */
   const unsigned char** bitmap_cache); /* May be NULL. */

#endif /* RDR_FONT_H */

