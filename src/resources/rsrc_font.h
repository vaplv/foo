#ifndef RSRC_FONT_H
#define RSRC_FONT_H

#include "resources/rsrc.h"
#include "resources/rsrc_error.h"
#include <stdbool.h>
#include <stddef.h>

struct rsrc_context;
struct rsrc_font;
struct rsrc_glyph;
struct rsrc_glyph_desc {
  struct {
    int x_min;
    int y_min;
    int x_max;
    int y_max;
  } bbox;
  size_t width;
  wchar_t character;
};

RSRC_API enum rsrc_error
rsrc_create_font
  (struct rsrc_context* ctxt,
   const char* path, /* May be NULL. */
   struct rsrc_font** out_font);

RSRC_API enum rsrc_error
rsrc_font_ref_get
  (struct rsrc_font* font);

RSRC_API enum rsrc_error
rsrc_font_ref_put
  (struct rsrc_font* font);

RSRC_API enum rsrc_error
rsrc_load_font
  (struct rsrc_font* font,
   const char* path);

RSRC_API enum rsrc_error
rsrc_font_size
  (struct rsrc_font* font,
   size_t width,  /* In Pixels. */
   size_t height); /* In Pixels. */

RSRC_API enum rsrc_error
rsrc_font_line_space
  (const struct rsrc_font* font,
   size_t* line_space);

RSRC_API enum rsrc_error
rsrc_is_font_scalable
  (const struct rsrc_font* font,
   bool* is_scalable);

RSRC_API enum rsrc_error
rsrc_font_glyph
  (struct rsrc_font* font,
   wchar_t ch,
   struct rsrc_glyph** glyph);

RSRC_API enum rsrc_error
rsrc_glyph_ref_get
  (struct rsrc_glyph* glyph);

RSRC_API enum rsrc_error
rsrc_glyph_ref_put
  (struct rsrc_glyph* glyph);

RSRC_API enum rsrc_error
rsrc_glyph_bitmap
  (struct rsrc_glyph* glyph,
   bool antialiasing, 
   size_t* width, /* May be NULL. */
   size_t* height, /* May be NULL. */
   size_t* bytes_per_pixel, /* May be NULL. */
   unsigned char* buffer); /* May be NULL. */

RSRC_API enum rsrc_error
rsrc_glyph_desc
  (const struct rsrc_glyph* glyph,
   struct rsrc_glyph_desc* desc);


#endif /* RSRC_FONT_H */

