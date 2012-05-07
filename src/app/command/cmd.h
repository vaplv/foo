#ifndef CMD_H
#define CMD_H

#include "app/command/cmd_error.h"

#ifndef NDEBUG
  #include <assert.h>
  #define CMD(func) assert(CMD_NO_ERROR == cmd_##func)
#else
  #define CMD(func) cmd_##func
#endif /* NDEBUG */

struct app;

/* Register several commands usefull to edit a project. */
extern enum cmd_error
cmd_setup_edit_commands
  (struct app* app);

extern enum cmd_error
cmd_release_edit_commands
  (struct app* app);

#endif /* CMD_H */

