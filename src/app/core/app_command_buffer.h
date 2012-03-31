#ifndef APP_COMMAND_BUFFER_H
#define APP_COMMAND_BUFFER_H

#include "app/core/app_error.h"

struct app;
struct app_command_buffer;

extern enum app_error
app_create_command_buffer
  (struct app* app,
   struct app_command_buffer** cmdbuf);

extern enum app_error
app_command_buffer_ref_get
  (struct app_command_buffer* cmdbuf);

extern enum app_error
app_command_buffer_ref_put
  (struct app_command_buffer* cmdbuf);

extern enum app_error
app_command_buffer_write_string
  (struct app_command_buffer* cmdbuf,
   const char* str);

extern enum app_error
app_command_buffer_write_char
  (struct app_command_buffer* cmdbuf,
   char ch);

extern enum app_error
app_command_buffer_write_backspace
  (struct app_command_buffer* cmdbuf);

extern enum app_error
app_command_buffer_write_suppr
  (struct app_command_buffer* cmdbuf);

extern enum app_error
app_command_buffer_move_cursor
  (struct app_command_buffer* cmdbuf,
   int i);

extern enum app_error
app_execute_command_buffer
  (struct app_command_buffer* cmdbuf);

extern enum app_error
app_clear_command_buffer
  (struct app_command_buffer* cmdbuf);

extern enum app_error
app_command_buffer_history_next
  (struct app_command_buffer* cmdbuf);

extern enum app_error
app_command_buffer_history_prev
  (struct app_command_buffer* cmdbuf);

extern enum app_error
app_command_buffer_clear_history
  (struct app_command_buffer* cmdbuf);

extern enum app_error
app_dump_command_buffer
  (struct app_command_buffer* cmdbuf,
   size_t* len, /* May be NULL. Do not include the null char. */
   char* buffer); /* May be NULL. */

#endif /* APP_COMMAND_BUFFER_H */

