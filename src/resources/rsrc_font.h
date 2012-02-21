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
  size_t width;
  size_t bitmap_left;
  size_t bitmap_top;
  wchar_t character;
};

extern enum rsrc_error
rsrc_create_font
  (struct rsrc_context* ctxt,
   const char* path, /* May be NULL. */
   struct rsrc_font** out_font);

extern enum rsrc_error
rsrc_free_font
  (struct rsrc_font* font);

extern enum rsrc_error
rsrc_load_font
  (struct rsrc_font* font,
   const char* path);

extern enum rsrc_error
rsrc_font_size
  (struct rsrc_font* font,
   size_t width,  /* In Pixels. */
   size_t height); /* In Pixels. */

extern enum rsrc_error
rsrc_font_line_space
  (const struct rsrc_font* font,
   size_t* line_space);

extern enum rsrc_error
rsrc_font_glyph
  (struct rsrc_font* font,
   wchar_t ch,
   struct rsrc_glyph** glyph);

extern enum rsrc_error
rsrc_free_glyph
  (struct rsrc_glyph* glyph);

extern enum rsrc_error
rsrc_glyph_bitmap
  (struct rsrc_glyph* glyph,
   bool antialiasing, 
   size_t* width, /* May be NULL. */
   size_t* height, /* May be NULL. */
   size_t* bytes_per_pixel, /* May be NULL. */
   unsigned char* buffer); /* May be NULL. */

extern enum rsrc_error
rsrc_glyph_desc
  (const struct rsrc_glyph* glyph,
   struct rsrc_glyph_desc* desc);


#endif /* RSRC_FONT_H */

