#include "app/core/regular/app_builtin_commands.h"
#include "app/core/regular/app_c.h"
#include "app/core/regular/app_command_c.h"
#include "app/core/app_command.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "app/core/app_world.h"
#include "renderer/rdr_term.h"
#include "renderer/rdr.h"
#include "stdlib/sl.h"
#include "stdlib/sl_flat_set.h"
#include "stdlib/sl_hash_table.h"
#include "sys/sys.h"
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
      APP_LOG_MSG(app->logger, "Model loaded `%s'\n", argv[2].value.string);
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
  "--all", "--models", "--model-instances", "--commands", NULL
};
static void
cmd_ls(struct app* app, size_t argc UNUSED, struct app_cmdarg* argv)
{
  size_t len = 0;
  size_t i = 0;
  bool all = false;

  assert(app != NULL
      && argc == 2
      && argv != NULL
      && argv[0].type == APP_CMDARG_STRING
      && argv[1].type == APP_CMDARG_STRING);

  all = 0 == strcmp("--all", argv[1].value.string);

  if(0 == strcmp("--commands", argv[1].value.string) || all) {
    const char** name_list = NULL;
    SL(flat_set_buffer
      (app->cmd.name_set, &len, NULL, NULL, (void**)&name_list));
    for(i = 0; i < len; ++i) {
      APP_LOG_MSG(app->logger, "%s\n", name_list[i]);
    }
    APP_LOG_MSG(app->logger, "[total %zu commands]%s", len, all ? "\n\n":"\n");
  }
  if(0 == strcmp("--models", argv[1].value.string) || all) {
    struct app_model** model_list = NULL;
    APP(get_registered_object_list
      (app, APP_MODEL, &len, (void**)&model_list));
    for(i = 0; i < len; ++i) {
      const char* name = NULL;
      APP(get_model_name(model_list[i], &name));
      APP_LOG_MSG(app->logger, "%s\n", name);
    }
    APP_LOG_MSG(app->logger, "[total %zu models]%s", len, all ? "\n\n":"\n");
  }
  if(0 == strcmp("--model-instances", argv[1].value.string) || all) {
    struct app_model_instance** instance_list = NULL;
    APP(get_registered_object_list
      (app, APP_MODEL_INSTANCE, &len, (void**)&instance_list));
    for(i = 0; i < len; ++i) {
      const char* name = NULL;
      APP(get_model_instance_name(instance_list[i], &name));
      APP_LOG_MSG(app->logger, "%s\n", name);
    }
    APP_LOG_MSG(app->logger, "[total %zu model instances]\n", len);
  }
}

static void
cmd_clear(struct app* app, size_t argc UNUSED, struct app_cmdarg* argv UNUSED)
{
  assert(app != NULL
      && argc == 1
      && argv != NULL
      && argv[0].type == APP_CMDARG_STRING);
  RDR(clear_term(app->term.render_term, RDR_TERM_STDOUT));
}

static void
cmd_spawn(struct app* app, size_t argc UNUSED, struct app_cmdarg* argv)
{
  bool b = false;
  assert(app != NULL
      && argc == 2
      && argv != NULL
      && argv[0].type == APP_CMDARG_STRING
      && argv[1].type == APP_CMDARG_STRING);

  APP(is_object_registered(app, APP_MODEL, &argv[1].value.string, &b));
  if(b == false) {
    APP_LOG_ERR
      (app->logger, "the model `%s' does not exist\n", argv[1].value.string);
  } else {
    struct app_model** mdl = NULL;
    struct app_model_instance* instance = NULL;
    enum app_error app_err = APP_NO_ERROR;

    APP(get_registered_object
      (app, APP_MODEL, &argv[1].value.string, (void**)&mdl));
    app_err = app_instantiate_model(app, *mdl, &instance);
    if(app_err != APP_NO_ERROR) {
      APP_LOG_ERR
        (app->logger, 
         "error instantiating the model `%s': %s\n", 
         argv[1].value.string,
         app_error_string(app_err));
    } else {
      app_err = app_world_add_model_instances(app->world, 1, &instance);
      if(app_err != APP_NO_ERROR) {
        const char* cstr = NULL;
        APP(get_model_instance_name(instance, &cstr));
        APP_LOG_ERR
          (app->logger,
           "error adding the instance `%s' to the world: %s\n",
           cstr,
           app_error_string(app_err));
        APP(model_instance_ref_put(instance));
      }
    }
  }
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

  CALL(app_add_command
    (app, "clear", cmd_clear, 0, NULL, "clear - clear the terminal screen\n"));
  CALL(app_add_command
    (app, "exit", cmd_exit, 0, NULL, "exit - cause the application to exit\n"));
  CALL(app_add_command
    (app, "help", cmd_help,
     1, APP_CMDARGV(APP_CMDARG_APPEND_STRING(NULL)),
     "help - give command informations\n"
     "Usage: help COMMAND\n"));
  CALL(app_add_command
    (app, "ls", cmd_ls,
     1, APP_CMDARGV(APP_CMDARG_APPEND_STRING(cmd_ls_options)),
     "ls - list application contents\n"
     "Usage: ls OPTION\n"
     "\t--all print all the listable objects\n"
     "\t--model-instances list the spawned model instances\n"
     "\t--models list the loaded models\n"
     "\t--commands list the registered commands\n"));
  CALL(app_add_command
    (app, "load", cmd_load,
     2, APP_CMDARGV
      (APP_CMDARG_APPEND_STRING(cmd_load_options),
       APP_CMDARG_APPEND_STRING(NULL)),
     "load - load resources\n"
     "Usage: load --model PATH\n"));
  CALL(app_add_command
    (app, "spawn", cmd_spawn,
     1, APP_CMDARGV(APP_CMDARG_APPEND_STRING(NULL)),
     "spawn - spawn a model into the world\n"
     "Usage: spawn MODEL_NAME\n"));

  #undef CALL

exit:
  return app_err;
error:
  goto exit;
}

