#ifndef CMD_ERROR_C_H
#define CMD_ERROR_C_H

#include "app/command/cmd_error.h"
#include "app/core/app_error.h"

extern enum cmd_error
app_to_cmd_error
  (enum app_error err);

#endif /* CMD_ERROR_C_H */

