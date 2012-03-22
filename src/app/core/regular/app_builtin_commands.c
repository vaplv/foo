#include "app/core/regular/app_c.h"
#include "app/core/regular/app_command_c.h"
#include "app/core/regular/app_builtin_commands.h"
#include "app/core/app_command.h"
#include "app/core/app_model.h"
#include "stdlib/sl.h"
#include "stdlib/sl_hash_table.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

/*******************************************************************************
 *
 * Command functions.
 *
 ******************************************************************************/
static void
cmd_exit(struct app* app, size_t argc UNUSED, struct app_cmdarg* argv UNUSED)
{
  assert(app != NULL
      && argc == 1
      && argv != NULL
      && argv[0].type == APP_CMDARG_STRING);
  app->post_exit = true;
}

static const char* cmd_load_options[] = {
  "--model", NULL
};
static void
cmd_load(struct app* app, size_t argc UNUSED, struct app_cmdarg* argv)
{
  struct app_model* mdl = NULL;
  enum app_error app_err = APP_NO_ERROR;
  assert(app != NULL
      && argc == 3
      && argv != NULL
      && argv[0].type == APP_CMDARG_STRING
      && argv[1].type == APP_CMDARG_STRING
      && argv[2].type == APP_CMDARG_STRING);

  if(0 == strcmp(argv[1].value.string, "--model")) {
    app_err = app_create_model(app, argv[2].value.string, &mdl);
    if(app_err == APP_NO_ERROR) {
      APP_LOG_MSG(app->logger, "Model loaded `%s'\n", argv[1].value.string);
    } else {
      APP_LOG_ERR(app->logger, "Model loading error\n");
    }
  } else {
    assert(false);
  }
}

static void
cmd_help(struct app* app, size_t argc UNUSED, struct app_cmdarg* argv)
{
  struct app_command** cmd = NULL;
  assert(app != NULL
      && argc == 2
      && argv != NULL
      && argv[0].type == APP_CMDARG_STRING
      && argv[1].type == APP_CMDARG_STRING);

  SL(hash_table_find(app->cmd.htbl, &argv[1].value.string, (void**)&cmd));
  if(!cmd) {
    APP_LOG_ERR(app->logger, "%s: command not found\n", argv[1].value.string);
  } else {
    if((*cmd)->description) {
      APP_LOG_MSG(app->logger, "%s", (*cmd)->description);
    } else {
      APP_LOG_MSG(app->logger, "none\n");
    }
  }
}

static const char* cmd_ls_options[] = {
  "--all", "--models", "--commands", NULL
};
static void
cmd_ls(struct app* app, size_t argc UNUSED, struct app_cmdarg* argv)
{
  assert(app != NULL
      && argc == 2
      && argv != NULL
      && argv[0].type == APP_CMDARG_STRING
      && argv[1].type == APP_CMDARG_STRING);
  APP_LOG_WARN
    (app->logger, "%s: command not implemented\n", argv[0].value.string);
}

/*******************************************************************************
 *
 * Builtin commands registration.
 *
 ******************************************************************************/
enum app_error
app_setup_builtin_commands(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  assert(app);

  #define CALL(func) if(APP_NO_ERROR != (app_err = func)) goto error

  CALL(app_add_command(app, "exit", cmd_exit, 0, NULL, NULL));
  CALL(app_add_command
    (app, "help", cmd_help,
     1, APP_CMDARGV(APP_CMDARG_APPEND_STRING(NULL)),
     "Usage: help COMMAND\n"));
  CALL(app_add_command
    (app, "load", cmd_load,
     2, APP_CMDARGV
      (APP_CMDARG_APPEND_STRING(cmd_load_options),
       APP_CMDARG_APPEND_STRING(NULL)),
     "Usage: load --model PATH\n"));
  CALL(app_add_command
    (app, "ls", cmd_ls,
     1, APP_CMDARGV(APP_CMDARG_APPEND_STRING(cmd_ls_options)),
     "Usage: ls OPTION\n"
     "\t--all print all the listable objects\n"
     "\t--models list the loaded models\n"
     "\t--commands list the registered commands\n"));

  #undef CALL

exit:
  return app_err;
error:
  goto exit;
}

