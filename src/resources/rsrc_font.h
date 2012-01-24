#ifndef RSRC_FONT_H
#define RSRC_FONT_H

#include "resources/rsrc.h"
#include "resources/rsrc_error.h"
#include <stddef.h>

struct rsrc_context;
struct rsrc_font;

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
rsrc_font_bitmap
  (const struct rsrc_font* font,
   unsigned long int ch,
   size_t* width, /* May be NULL. */
   size_t* height, /* May be NULL. */
   size_t* bytes_per_pixel, /* May be NULL. */
   unsigned char* buffer); /* May be NULL. */

#endif /* RSRC_FONT_H */

