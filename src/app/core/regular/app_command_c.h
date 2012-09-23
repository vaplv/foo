#ifndef APP_COMMAND_C_H
#define APP_COMMAND_C_H

#include "app/core/app_error.h"
#include "sys/sys.h"

struct app;

LOCAL_SYM enum app_error
app_init_command_system
  (struct app* app);

LOCAL_SYM enum app_error
app_shutdown_command_system
  (struct app* app);

#endif /* APP_COMMAND2_C_H */

