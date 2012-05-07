#include "app/command/regular/cmd_load_save.h"
#include "app/command/regular/cmd_move.h"
#include "app/command/regular/cmd_object_management.h"
#include "app/command/cmd.h"

#include "sys/sys.h"

EXPORT_SYM enum cmd_error
cmd_setup_edit_commands(struct app* app)
{
  enum cmd_error cmd_err = CMD_NO_ERROR;

  if(!app) {
    cmd_err = CMD_INVALID_ARGUMENT;
    goto error;
  }

  #define CALL(func) \
    do { \
      if(CMD_NO_ERROR != (cmd_err = func)) \
        goto error; \
    } while(0)

  CALL(cmd_setup_move_commands(app));
  CALL(cmd_setup_object_management_commands(app));
  CALL(cmd_setup_load_save_commands(app));

  #undef CALL

exit:
  return cmd_err;
error:
  if(app)
    CMD(release_edit_commands(app));
  goto exit;
}

EXPORT_SYM enum cmd_error
cmd_release_edit_commands(struct app* app)
{
  if(!app)
    return CMD_INVALID_ARGUMENT;
  CMD(release_move_commands(app));
  CMD(release_object_management_commands(app));
  CMD(release_load_save_commands(app));
  return CMD_NO_ERROR;
}
