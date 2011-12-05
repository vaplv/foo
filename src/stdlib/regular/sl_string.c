#include "stdlib/sl_string.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <string.h>

#define STR_BUFFER_SIZE 16

struct sl_string {
  struct mem_allocator* allocator;
  size_t len;
  char* cstr;
  char buffer[STR_BUFFER_SIZE];
};

EXPORT_SYM enum sl_error
sl_create_string
  (const char* val,
   struct mem_allocator* specific_allocator,
   struct sl_string** out_str)
{
  struct mem_allocator* allocator = NULL;
  struct sl_string* str = NULL;
  size_t val_len = 0;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!out_str) {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }
  allocator = specific_allocator ? specific_allocator : &mem_default_allocator;
  str = MEM_CALLOC(allocator, 1, sizeof(struct sl_string));
  if(!str) {
    sl_err = SL_MEMORY_ERROR;
    goto error;
  }
  str->allocator = allocator;
  str->buffer[0] = '\0';

  val_len = val ? strlen(val) + 1 : 0;
  if(val_len <= STR_BUFFER_SIZE) {
    str->len = STR_BUFFER_SIZE;
    str->cstr = str->buffer;
  } else {
    str->cstr = MEM_ALLOC(allocator, sizeof(char) * val_len);
    if(!str->buffer) {
      sl_err = SL_MEMORY_ERROR;
      goto error;
    }
    str->len = val_len;
  }
  if(val)
    str->cstr = strncpy(str->cstr, val, val_len);

exit:
  if(out_str)
    *out_str = str;
  return sl_err;
error:
  if(str) {
    if(str->cstr != str->buffer)
      MEM_FREE(allocator, str->cstr);
    MEM_FREE(allocator, str);
    str = NULL;
  }
  goto exit;
}

EXPORT_SYM enum sl_error
sl_free_string(struct sl_string* str)
{
  enum sl_error sl_err = SL_NO_ERROR;

  if(!str)
    return SL_INVALID_ARGUMENT;

  sl_err = sl_clear_string(str);
  assert(sl_err == SL_NO_ERROR);
  MEM_FREE(str->allocator, str);

  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_clear_string(struct sl_string* str)
{
  if(!str)
    return SL_INVALID_ARGUMENT;

  if(str->buffer != str->cstr)
    MEM_FREE(str->allocator, str->cstr);

  str->len = STR_BUFFER_SIZE;
  str->cstr = str->buffer;
  str->buffer[0] = '\0';

  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_is_string_empty(struct sl_string* str, bool* is_empty)
{
  if(!str || !is_empty)
    return SL_INVALID_ARGUMENT;
  *is_empty = str->cstr[0] == '\0';
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_string_set(struct sl_string* str, const char* val)
{
  char* old_cstr = NULL;
  char* cstr = NULL;
  size_t val_len = 0;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!str || !val) {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }

  val_len = strlen(val) + 1;
  if(val_len > str->len) {
    cstr = MEM_ALLOC(str->allocator, sizeof(char) * val_len);
    if(!cstr) {
      sl_err = SL_MEMORY_ERROR;
      goto error;
    }
    old_cstr = str->cstr;
    str->cstr = cstr;
    str->len = val_len;
  }
  str->cstr = strncpy(str->cstr, val, val_len);

exit:
  if(old_cstr && old_cstr != str->buffer)
    MEM_FREE(str->allocator, old_cstr);
  return sl_err;

error:
  if(cstr) {
    MEM_FREE(str->allocator, cstr);
    cstr = NULL;
  }
  if(old_cstr) {
    str->cstr = old_cstr;
    old_cstr = NULL;
  }
  goto exit;
}

EXPORT_SYM enum sl_error
sl_string_get(struct sl_string* str, const char** cstr)
{
  if(!str || !cstr)
    return SL_INVALID_ARGUMENT;
  *cstr = str->cstr;
  return SL_NO_ERROR;
}


