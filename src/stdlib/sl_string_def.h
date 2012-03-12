#undef SL_STRING_STD
#undef SL_STRING_WIDE
#define SL_STRING_STD 0
#define SL_STRING_WIDE 1

#ifndef SL_STRING_TYPE
  #error "Undefined string type."
#elif (SL_STRING_TYPE != SL_STRING_STD) && (SL_STRING_TYPE != SL_STRING_WIDE)
  #error "Unexpected string type."
#endif

/* Wrap the function name with respect to the string type. */
#ifndef SL_STRING_DEF_H
#define SL_STRING_DEF_H
  #define SL_STRING(type) struct CONCAT(sl_, type)
  #define SL_STRING_CHAR(type) CONCAT(type, _char__)
  #define SL_CREATE_STRING(type) CONCAT(sl_create_, type)
  #define SL_FREE_STRING(type) CONCAT(sl_free_, type)
  #define SL_CLEAR_STRING(type) CONCAT(sl_clear_, type)
  #define SL_IS_STRING_EMPTY(type) CONCAT(CONCAT(sl_is_, type), _empty)
  #define SL_STRING_SET(type) CONCAT(CONCAT(sl_, type), _set)
  #define SL_STRING_GET(type) CONCAT(CONCAT(sl_, type), _get)
  #define SL_STRING_LENGTH(type) CONCAT(CONCAT(sl_, type), _length)
  #define SL_STRING_INSERT(type) CONCAT(CONCAT(sl_, type), _insert)
  #define SL_STRING_INSERT_CHAR(type) CONCAT(CONCAT(sl_, type), _insert_char)
  #define SL_STRING_APPEND(type) CONCAT(CONCAT(sl_, type), _append)
  #define SL_STRING_APPEND_CHAR(type) CONCAT(CONCAT(sl_, type), _append_char)
  #define SL_STRING_ERASE(type) CONCAT(CONCAT(sl_, type), _erase)
  #define SL_STRING_ERASE_CHAR(type) CONCAT(CONCAT(sl_, type), _erase_char)
  #define SL_STRING_CAPACITY(type) CONCAT(CONCAT(sl_, type), _capacity)
#endif /* SL_STRING_DEF_H */

#if (defined(SL_STRING_STD_DEF_H) || (SL_STRING_TYPE != SL_STRING_STD)) \
&&  (defined(SL_STRING_WIDE_DEF_H) || (SL_STRING_TYPE != SL_STRING_WIDE))
  #undef SL_STRING_STD
  #undef SL_STRING_WIDE
  #define SL_STRING_STD string
  #define SL_STRING_WIDE wstring
#else
  #include "stdlib/sl_error.h"
  #include <sys/sys.h>
  #include <stdbool.h>
  #include <stddef.h>

  /* Define the char type of the string. */
  #if SL_STRING_TYPE == SL_STRING_STD
    #define SL_STRING_STD_DEF_H
    typedef char string_char__;
  #else
    #define SL_STRING_WIDE_DEF_H
    #include <wchar.h>
    typedef wchar_t wstring_char__;
  #endif

  #undef SL_STRING_STD
  #undef SL_STRING_WIDE
  #define SL_STRING_STD string
  #define SL_STRING_WIDE wstring

struct mem_allocator;
SL_STRING(SL_STRING_TYPE);

extern enum sl_error
SL_CREATE_STRING(SL_STRING_TYPE)
  (const SL_STRING_CHAR(SL_STRING_TYPE)* val,
   struct mem_allocator* allocator, /* May be NULL. */
   SL_STRING(SL_STRING_TYPE)** str); /* May be NULL. */

extern enum sl_error
SL_FREE_STRING(SL_STRING_TYPE)
  (SL_STRING(SL_STRING_TYPE)* str);

extern enum sl_error
SL_CLEAR_STRING(SL_STRING_TYPE)
  (SL_STRING(SL_STRING_TYPE)* str);

extern enum sl_error
SL_IS_STRING_EMPTY(SL_STRING_TYPE)
  (SL_STRING(SL_STRING_TYPE)* str,
   bool* is_empty);

extern enum sl_error
SL_STRING_SET(SL_STRING_TYPE)
  (SL_STRING(SL_STRING_TYPE)* str,
   const SL_STRING_CHAR(SL_STRING_TYPE)* val);

extern enum sl_error
SL_STRING_GET(SL_STRING_TYPE)
  (SL_STRING(SL_STRING_TYPE)* str,
   const SL_STRING_CHAR(SL_STRING_TYPE)** cstr);

extern enum sl_error
SL_STRING_LENGTH(SL_STRING_TYPE)
  (SL_STRING(SL_STRING_TYPE)* str,
   size_t* len);

extern enum sl_error
SL_STRING_INSERT(SL_STRING_TYPE)
  (SL_STRING(SL_STRING_TYPE)* str,
   size_t id,
   const SL_STRING_CHAR(SL_STRING_TYPE)* cstr);

extern enum sl_error
SL_STRING_INSERT_CHAR(SL_STRING_TYPE)
  (SL_STRING(SL_STRING_TYPE)* str,
   size_t id,
   SL_STRING_CHAR(SL_STRING_TYPE) ch);

extern enum sl_error
SL_STRING_APPEND(SL_STRING_TYPE)
  (SL_STRING(SL_STRING_TYPE)* str,
   const SL_STRING_CHAR(SL_STRING_TYPE)* cstr);

extern enum sl_error
SL_STRING_APPEND_CHAR(SL_STRING_TYPE)
  (SL_STRING(SL_STRING_TYPE)* str,
   SL_STRING_CHAR(SL_STRING_TYPE) ch);

extern enum sl_error
SL_STRING_ERASE(SL_STRING_TYPE)
  (SL_STRING(SL_STRING_TYPE)* str,
   size_t id,
   size_t len);

extern enum sl_error
SL_STRING_ERASE_CHAR(SL_STRING_TYPE)
  (SL_STRING(SL_STRING_TYPE)* str,
   size_t id);

extern enum sl_error
SL_STRING_CAPACITY(SL_STRING_TYPE)
  (SL_STRING(SL_STRING_TYPE)* str,
   size_t capacity);

#endif /* SL_STRING_STD_DEF_H || SL_STRING_WIDE_DEF_H */

