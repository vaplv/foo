#ifndef RDR_FONT_C_H
#define RDR_FONT_C_H

#include "renderer/rdr_error.h"
#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

enum rdr_font_signal {
  RDR_FONT_SIGNAL_UPDATE_DATA,
  RDR_NB_FONT_SIGNALS
};

struct rdr_glyph {
  size_t width;
  struct {
    float x;
    float y;
  } tex[2], pos[2];
};

struct rb_tex2d;
struct rdr_font;

LOCAL_SYM enum rdr_error
rdr_get_font_glyph
  (struct rdr_font* font,
   wchar_t character,
   struct rdr_glyph* desc);

LOCAL_SYM enum rdr_error
rdr_get_font_texture
  (struct rdr_font* font,
   struct rb_tex2d** tex);

LOCAL_SYM enum rdr_error
rdr_font_attach_callback
  (const struct rdr_font* font,
   enum rdr_font_signal signal,
   void (*callback)(struct rdr_font*, void*),
   void* data); /* May be NULL. */

LOCAL_SYM enum rdr_error
rdr_font_detach_callback
  (const struct rdr_font* font,
   enum rdr_font_signal signal,
   void (*callback)(struct rdr_font*, void*),
   void* data); /* May be NULL. */

LOCAL_SYM enum rdr_error
rdr_is_font_callback_attached
  (const struct rdr_font* font,
   enum rdr_font_signal signal,
   void (*callback)(struct rdr_font*, void*),
   void* data, /* May be NULL. */
   bool* is_attached);

#endif /* RDR_FONT_C_H */

