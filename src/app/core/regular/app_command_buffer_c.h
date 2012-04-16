#ifndef APP_COMMAND_BUFFER_C_H
#define APP_COMMAND_BUFFER_C_H

#include "app/core/app_error.h"

struct app;
struct app_command_buffer;

/* Ensure that the created buffer does not take the ownership of the app. */
extern enum app_error
app_regular_create_command_buffer
  (struct app* app,
   struct app_command_buffer** cmdbuf);

/* Does not release the ownership of the app. */
extern enum app_error
app_regular_command_buffer_ref_put
  (struct app_command_buffer* cmdbuf);

extern enum app_error
app_get_command_buffer_string
  (struct app_command_buffer* buf,
   size_t* cursor, /* May be NULL. */
   const char** str) /* May be NULL. */;

extern enum app_error
app_command_buffer_completion
  (struct app_command_buffer* buf,
   size_t* completion_list_len, /* May be NULL. */
   const char** completion_list[]); /* May be NULL. */

#endif /* APP_COMMAND_BUFFER_C_H */

