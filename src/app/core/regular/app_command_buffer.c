#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/app_command.h"
#include "app/core/app_command_buffer.h"
#include "stdlib/sl.h"
#include "stdlib/sl_string.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include "sys/mem_allocator.h"
#include <stdlib.h>
#include <string.h>

struct app_command_buffer {
  struct ref ref;
  struct app* app;
  struct sl_string* str;
  size_t cursor;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_command_buffer(struct ref* ref)
{
  struct app* app = NULL;
  struct app_command_buffer* buf = NULL;
  assert(ref);

  buf = CONTAINER_OF(ref, struct app_command_buffer, ref);
  if(buf->str)
    SL(free_string(buf->str));
  app = buf->app;
  MEM_FREE(app->allocator, buf);
}

/*******************************************************************************
 *
 * Command buffer functions.
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_create_command_buffer
  (struct app* app,
   struct app_command_buffer** cmdbuf)
{
  struct app_command_buffer* buf = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app || !cmdbuf) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  buf = MEM_CALLOC(app->allocator, 1, sizeof(struct app_command_buffer));
  if(!buf) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  ref_init(&buf->ref);
  buf->app = app;
  buf->cursor = 0;
  sl_err = sl_create_string(NULL, app->allocator, &buf->str);
  if(SL_NO_ERROR != sl_err) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
exit:
  if(cmdbuf)
    *cmdbuf = buf;
  return app_err;
error:
  if(buf) {
    APP(command_buffer_ref_put(buf));
    buf = NULL;
  }
  goto exit;
}

EXPORT_SYM enum app_error
app_command_buffer_ref_get(struct app_command_buffer* buf)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  ref_get(&buf->ref);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_command_buffer_ref_put(struct app_command_buffer* buf)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  ref_put(&buf->ref, release_command_buffer);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_command_buffer_write_string
  (struct app_command_buffer* buf,
   const char* str)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!buf || !str) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_string_insert(buf->str, buf->cursor, str);
  if(SL_NO_ERROR != sl_err) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  buf->cursor += strlen(str);
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_command_buffer_write_char(struct app_command_buffer* buf, char ch)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!buf) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_string_insert_char(buf->str, buf->cursor, ch);
  if(SL_NO_ERROR != sl_err) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  ++buf->cursor;
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_command_buffer_write_backspace(struct app_command_buffer* buf)
{
  enum app_error app_err = APP_NO_ERROR;
  if(!buf) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  if(buf->cursor > 0) {
    --buf->cursor;
    SL(string_erase_char(buf->str, buf->cursor));
  }
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_command_buffer_write_suppr(struct app_command_buffer* buf)
{
  size_t len = 0;
  enum app_error app_err = APP_NO_ERROR;
  if(!buf) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(string_length(buf->str, &len));
  if(buf->cursor != len) {
    ++buf->cursor;
    app_err = app_command_buffer_write_backspace(buf);
  }
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_command_buffer_move_cursor(struct app_command_buffer* buf, int i)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  if(i <= 0) {
    const size_t t = (size_t)abs(i);
    buf->cursor -= MIN(t, buf->cursor);
  } else {
    size_t len = 0;
    const size_t t = (size_t)i;
    SL(string_length(buf->str, &len));
    buf->cursor += MIN(t, len - buf->cursor);
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_clear_command_buffer(struct app_command_buffer* buf)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  SL(clear_string(buf->str));
  buf->cursor = 0;
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_execute_command_buffer(struct app_command_buffer* buf)
{
  const char* cstr = NULL;
  size_t len = 0;
  enum app_error app_err = APP_NO_ERROR;

  if(!buf) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(string_length(buf->str, &len));
  if(len) {
    SL(string_get(buf->str, &cstr));
    app_err = app_execute_command(buf->app, cstr);
    if(app_err != APP_NO_ERROR)
      goto error;
    APP(clear_command_buffer(buf));
  }
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_dump_command_buffer
  (struct app_command_buffer* buf,
   size_t* len,
   char* buffer)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  if(len) {
    SL(string_length(buf->str, len));
    *len += (*len != 0); /* NULL char. */
  }
  if(buffer) {
    const char* cstr = NULL;
    SL(string_get(buf->str, &cstr));
    strcpy(buffer, cstr);
  }
  return APP_NO_ERROR;
}

