#ifndef APP_COMMAND_C_H
#define APP_COMMAND_C_H

#include "app/core/app_error.h"
#include "sys/sys.h"
#include <assert.h>

struct app_command;

extern enum app_error
app_init_command_system
  (struct app* app);

extern enum app_error
app_shutdown_command_system
  (struct app* app);

#endif /* APP_COMMAND_C_H */

