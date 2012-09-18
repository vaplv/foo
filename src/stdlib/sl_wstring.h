#ifndef SL_WSTRING_H
#define SL_WSTRING_H

#define SL_STRING_TYPE SL_STRING_WIDE 
  #include "stdlib/sl_string.h.def"
#undef SL_STRING_TYPE

SL_API enum sl_error
sl_wstring_insert_cstr
  (struct sl_wstring* str,
   size_t id,
   const char* cstr);

SL_API enum sl_error
sl_wstring_append_cstr
  (struct sl_wstring* str,
   const char* cstr);

#endif /* SL_WSTRING_H */

