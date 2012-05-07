#ifndef CMD_LOAD_SAVE_H
#define CMD_LOAD_SAVE_H

#include "app/command/cmd_error.h"

struct app;

extern enum cmd_error
cmd_setup_load_save_commands
  (struct app* app);

extern enum cmd_error
cmd_release_load_save_commands
  (struct app* app);

#endif /* CMD_LOAD_SAVE_H */

