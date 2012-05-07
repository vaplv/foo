#include "app/command/regular/cmd_error_c.h"

enum cmd_error
app_to_cmd_error(enum app_error err)
{
  enum cmd_error cmd_err = CMD_NO_ERROR;
  switch(err) {
    case APP_INVALID_ARGUMENT:
      cmd_err = CMD_INVALID_ARGUMENT;
      break;
    case APP_MEMORY_ERROR:
      cmd_err = CMD_MEMORY_ERROR;
      break;
    case APP_NO_ERROR:
      cmd_err = CMD_NO_ERROR;
    default:
      cmd_err = CMD_UNKNOWN_ERROR;
      break;
  }
  return cmd_err;
}
