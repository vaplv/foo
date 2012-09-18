#ifndef RDR_TERM_H
#define RDR_TERM_H

#include "renderer/rdr.h"
#include "renderer/rdr_error.h"
#include <stddef.h>

/* Built-in terminal colors. */
static const unsigned char RDR_TERM_COLOR_BLACK[3]={0x00, 0x00, 0x00};
static const unsigned char RDR_TERM_COLOR_RED[3]={0xCD, 0x00, 0x00};
static const unsigned char RDR_TERM_COLOR_GREEN[3]={0x00, 0xCD, 0x00};
static const unsigned char RDR_TERM_COLOR_YELLOW[3]={0xCD, 0xCD, 0x00};
static const unsigned char RDR_TERM_COLOR_BLUE[3]={0x00, 0x00, 0xEE};
static const unsigned char RDR_TERM_COLOR_MAGENTA[3]={0xCD, 0x00, 0xCD};
static const unsigned char RDR_TERM_COLOR_CYAN[3]={0x00, 0xCD, 0xCD};
static const unsigned char RDR_TERM_COLOR_WHITE[3]={0xE5, 0xE5, 0xE5};
static const unsigned char RDR_TERM_COLOR_BRIGHT_BLACK[3]={0x7F, 0x7F, 0x7F};
static const unsigned char RDR_TERM_COLOR_BRIGHT_RED[3]={0xFF, 0x00, 0x00};
static const unsigned char RDR_TERM_COLOR_BRIGHT_GREEN[3]={0x00, 0xFF, 0x00};
static const unsigned char RDR_TERM_COLOR_BRIGHT_YELLOW[3]={0xFF, 0xFF, 0x00};
static const unsigned char RDR_TERM_COLOR_BRIGHT_BLUE[3]={0x5C, 0x5C, 0xFF};
static const unsigned char RDR_TERM_COLOR_BRIGHT_MAGENTA[3]={0xFF, 0x00, 0xFF};
static const unsigned char RDR_TERM_COLOR_BRIGHT_CYAN[3]={0x00, 0xFF, 0xFF};
static const unsigned char RDR_TERM_COLOR_BRIGHT_WHITE[3]={0xFF, 0xFF, 0xFF};

struct rdr_font;
struct rdr_system;
struct rdr_term;

enum rdr_term_output {
  RDR_TERM_STDOUT,
  RDR_TERM_CMDOUT,
  RDR_TERM_PROMPT
};

RDR_API enum rdr_error
rdr_create_term
  (struct rdr_system* sys,
   struct rdr_font* font,
   size_t width,
   size_t height,
   struct rdr_term** term);

RDR_API enum rdr_error
rdr_term_ref_get
  (struct rdr_term* term);

RDR_API enum rdr_error
rdr_term_ref_put
  (struct rdr_term* console);

RDR_API enum rdr_error
rdr_term_font
  (struct rdr_term* term,
   struct rdr_font* font);

RDR_API enum rdr_error
rdr_term_translate_cursor
  (struct rdr_term* term,
   int x);

RDR_API enum rdr_error
rdr_term_print_wstring
  (struct rdr_term* term,
   enum rdr_term_output output,
   const wchar_t* str,
   const unsigned char color[3]);

RDR_API enum rdr_error
rdr_term_print_string
  (struct rdr_term* term,
   enum rdr_term_output output,
   const char* str,
   const unsigned char color[3]);

RDR_API enum rdr_error
rdr_term_print_wchar
  (struct rdr_term* term,
   enum rdr_term_output output,
   wchar_t ch,
   const unsigned char color[3]);

RDR_API enum rdr_error
rdr_clear_term
  (struct rdr_term* term,
   enum rdr_term_output output);

RDR_API enum rdr_error
rdr_term_write_backspace
  (struct rdr_term* term);

RDR_API enum rdr_error
rdr_term_write_tab
  (struct rdr_term* term);

RDR_API enum rdr_error
rdr_term_write_return
  (struct rdr_term* term);

RDR_API enum rdr_error
rdr_term_write_suppr
  (struct rdr_term* term);

RDR_API enum rdr_error
rdr_term_dump
  (struct rdr_term* term,
   size_t* len, /* May be NULL. Do not include the null. */
   wchar_t* buffer); /* May be NULL. */

#endif /* RDR_TERM_H */

