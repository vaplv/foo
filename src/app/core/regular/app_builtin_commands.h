#ifndef APP_BUILTIN_COMMANDS_H
#define APP_BUILTIN_COMMANDS_H

#include "app/core/app_error.h"
#include "sys/sys.h"

struct app;

LOCAL_SYM enum app_error
app_setup_builtin_commands
  (struct app* app);

#endif /* APP_BUILTIN_COMMANDS_H */

