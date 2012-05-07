#ifndef CMD_OBJECT_MANAGEMENT_H
#define CMD_OBJECT_MANAGEMENT_H

#include "app/command/cmd_error.h"

struct app;

extern enum cmd_error
cmd_setup_object_management_commands
  (struct app* app);

extern enum cmd_error
cmd_release_object_management_commands
  (struct app* app);

#endif /* CMD_OBJECT_MANAGEMENT_H */

