#ifndef APP_COMMAND_BUFFER_H
#define APP_COMMAND_BUFFER_H

#include "app/core/app.h"
#include "app/core/app_error.h"

struct app;
struct app_command_buffer;

APP_API enum app_error
app_create_command_buffer
  (struct app* app,
   struct app_command_buffer** cmdbuf);

APP_API enum app_error
app_command_buffer_ref_get
  (struct app_command_buffer* cmdbuf);

APP_API enum app_error
app_command_buffer_ref_put
  (struct app_command_buffer* cmdbuf);

APP_API enum app_error
app_command_buffer_write_string
  (struct app_command_buffer* cmdbuf,
   const char* str);

APP_API enum app_error
app_command_buffer_write_char
  (struct app_command_buffer* cmdbuf,
   char ch);

APP_API enum app_error
app_command_buffer_write_backspace
  (struct app_command_buffer* cmdbuf);

APP_API enum app_error
app_command_buffer_write_suppr
  (struct app_command_buffer* cmdbuf);

APP_API enum app_error
app_command_buffer_move_cursor
  (struct app_command_buffer* cmdbuf,
   int i);

APP_API enum app_error
app_execute_command_buffer
  (struct app_command_buffer* cmdbuf);

APP_API enum app_error
app_clear_command_buffer
  (struct app_command_buffer* cmdbuf);

APP_API enum app_error
app_command_buffer_history_next
  (struct app_command_buffer* cmdbuf);

APP_API enum app_error
app_command_buffer_history_prev
  (struct app_command_buffer* cmdbuf);

APP_API enum app_error
app_command_buffer_clear_history
  (struct app_command_buffer* cmdbuf);

APP_API enum app_error
app_dump_command_buffer
  (struct app_command_buffer* cmdbuf,
   size_t* len, /* May be NULL. Do not include the null char. */
   char* buffer); /* May be NULL. */

APP_API enum app_error
app_command_buffer_completion
  (struct app_command_buffer* buf,
   size_t* completion_list_len, /* May be NULL. */
   /* List of available completion values. May be NULL. */
   const char** completion_list[]); 

#endif /* APP_COMMAND_BUFFER_H */

