#ifndef SL_STRING_H
#define SL_STRING_H

#include "stdlib/sl_error.h"
#include <stdbool.h>

struct mem_allocator;
struct sl_string;

extern enum sl_error
sl_create_string
  (const char* val, /* May be NULL. */
   struct mem_allocator* allocator, /* May be NULL. */
   struct sl_string** str);

extern enum sl_error
sl_free_string
  (struct sl_string* str);

extern enum sl_error
sl_clear_string
  (struct sl_string* str);

extern enum sl_error
sl_is_string_empty
  (struct sl_string* str,
   bool* is_empty);

extern enum sl_error
sl_string_set
  (struct sl_string* str,
   const char* val);

extern enum sl_error
sl_string_get
  (struct sl_string* str,
   const char** cstr);

#endif /* SL_STRING_H */

