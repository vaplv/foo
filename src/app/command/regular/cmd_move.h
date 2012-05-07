#ifndef CMD_MOVE_H
#define CMD_MOVE_H

#include "app/command/cmd_error.h"

struct app;

extern enum cmd_error
cmd_setup_move_commands
  (struct app* app);

extern enum cmd_error
cmd_release_move_commands
  (struct app* app);

#endif /* CMD_MOVE_H */
