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

#endif /* APP_COMMAND_BUFFER_C_H */

