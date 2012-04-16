#ifndef APP_COMMAND_C_H
#define APP_COMMAND_C_H

#include "app/core/app_command.h"
#include "app/core/app_error.h"
#include "sys/sys.h"
#include <assert.h>
#include <stddef.h>

struct app;

struct app_command {
  char* description;
  void(*func)(struct app*, size_t, struct app_cmdarg*);
  size_t argc;
  struct app_cmdarg_desc arg_desc_list[];
};

extern enum app_error
app_init_command_system
  (struct app* app);

extern enum app_error
app_shutdown_command_system
  (struct app* app);

extern enum app_error
app_get_command
  (struct app* app,
   const char* name, 
   struct app_command** cmd);

#endif /* APP_COMMAND_C_H */

