#ifndef RDR_TERM_H
#define RDR_TERM_H

#include "renderer/rdr_error.h"
#include <stddef.h>

struct rdr_font;
struct rdr_system;
struct rdr_term;

extern enum rdr_error
rdr_create_term
  (struct rdr_system* sys,
   struct rdr_font* font,
   size_t width,
   size_t height,
   struct rdr_term** term);

extern enum rdr_error
rdr_term_ref_get
  (struct rdr_term* term);

extern enum rdr_error
rdr_term_ref_put
  (struct rdr_term* console);

extern enum rdr_error
rdr_term_font
  (struct rdr_term* term,
   struct rdr_font* font);

extern enum rdr_error
rdr_term_translate_cursor
  (struct rdr_term* term,
   int x);

extern enum rdr_error
rdr_term_write_string
  (struct rdr_term* term,
   const wchar_t* str);

extern enum rdr_error
rdr_term_write_char
  (struct rdr_term* term,
   wchar_t ch);

extern enum rdr_error
rdr_term_write_backspace
  (struct rdr_term* term);

extern enum rdr_error
rdr_term_write_tab
  (struct rdr_term* term);

extern enum rdr_error
rdr_term_write_return
  (struct rdr_term* term);

extern enum rdr_error
rdr_term_write_suppr
  (struct rdr_term* term);

extern enum rdr_error
rdr_term_dump
  (const struct rdr_term* term,
   size_t* len, /* May be NULL. Include the null char. */
   wchar_t* buffer); /* May be NULL. */

#endif /* RDR_TERM_H */

