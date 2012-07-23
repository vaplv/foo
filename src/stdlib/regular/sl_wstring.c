#define SL_STRING_TYPE SL_STRING_WIDE
  #include "stdlib/regular/sl_string.c.def"
#undef SL_STRING_TYPE

#include "stdlib/sl_wstring.h"
#include <stdlib.h>

EXPORT_SYM enum sl_error
sl_wstring_insert_cstr(struct sl_wstring* str, size_t id, const char* cstr)
{
  enum sl_error sl_err = SL_NO_ERROR;

  if(!str
  || !cstr
  || id > str->len
  || IS_MEMORY_OVERLAPPED(str->cstr, str->allocated, cstr, strlen(cstr)+1)) {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }
  if(id == str->len) {
    sl_err = sl_wstring_append_cstr(str, cstr);
    if(sl_err != SL_NO_ERROR)
      goto error;
  } else {
    const size_t len = strlen(cstr);
    sl_err = ensure_allocated(str, len + str->len + 1, true);
    if(sl_err != SL_NO_ERROR)
      goto error;
    memmove
      (str->cstr + id + len,
       str->cstr + id,
       (str->len - id) * sizeof(wchar_t));
    mbstowcs(str->cstr + id, cstr, len);
    str->len = str->len + len;
    str->cstr[str->len] = L'\0';
  }
exit:
  return sl_err;
error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_wstring_append_cstr(struct sl_wstring* str, const char* cstr)
{
  enum sl_error sl_err = SL_NO_ERROR;
  const size_t len = cstr ? strlen(cstr) : 0;

  if(!str
  || !cstr
  || IS_MEMORY_OVERLAPPED(str->cstr, str->allocated, cstr, len + 1)) {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = ensure_allocated(str, len + str->len + 1, true);
  if(sl_err != SL_NO_ERROR)
    goto error;
  mbstowcs(str->cstr + str->len, cstr, len);
  str->len = str->len + len;
  str->cstr[str->len] = L'\0';

exit:
  return sl_err;
error:
  goto exit;
}
