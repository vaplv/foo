#include "app/command/regular/cmd_c.h"
#include "app/command/regular/cmd_error_c.h"
#include "app/command/regular/cmd_object_management.h"
#include "app/command/cmd.h"
#include "app/core/app.h"
#include "app/core/app_command.h"
#include "app/core/app_error.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "app/core/app_world.h"

/*******************************************************************************
 *
 * Command functions.
 *
 ******************************************************************************/
static void
rename_model
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv)
{
  struct app_model* mdl = NULL;
  enum { CMD_NAME, OLD_MODEL_NAME, NEW_MODEL_NAME, ARGC };

  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[OLD_MODEL_NAME]->type == APP_CMDARG_STRING
      && argv[NEW_MODEL_NAME]->type == APP_CMDARG_STRING);

  APP(get_model(app, CMD_ARGVAL(argv, OLD_MODEL_NAME).data.string, &mdl));
  if(mdl) {
    APP(set_model_name(mdl, CMD_ARGVAL(argv, NEW_MODEL_NAME).data.string));
  } else {
    APP(log(app, APP_LOG_ERROR,
      "the model `%s' does not exist\n",
      CMD_ARGVAL(argv, OLD_MODEL_NAME).data.string));
  }
}

static void
rename_instance
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv)
{
  struct app_model_instance* instance = NULL;
  enum { CMD_NAME, OLD_NAME, NEW_NAME, ARGC };

  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[OLD_NAME]->type == APP_CMDARG_STRING
      && argv[NEW_NAME]->type == APP_CMDARG_STRING);

  APP(get_model_instance
    (app, CMD_ARGVAL(argv, OLD_NAME).data.string, &instance));
  if(instance) {
    APP(set_model_instance_name
      (instance, CMD_ARGVAL(argv, NEW_NAME).data.string));
  } else {
    APP(log(app, APP_LOG_ERROR,
      "the instance `%s' does not exist\n",
      CMD_ARGVAL(argv, OLD_NAME).data.string));
  }
}

static void
rm_instance
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv)
{
  struct app_model_instance* instance = NULL;
  enum { CMD_NAME, INSTANCE_NAME, ARGC };

  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[INSTANCE_NAME]->type == APP_CMDARG_STRING);

  APP(get_model_instance
    (app, CMD_ARGVAL(argv, INSTANCE_NAME).data.string, &instance));
  if(instance != NULL) {
    APP(remove_model_instance(instance));
  } else {
    APP(log(app, APP_LOG_ERROR,
      "the instance `%s' does not exist\n",
      CMD_ARGVAL(argv, INSTANCE_NAME).data.string));
  }
}

static void
rm_model
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv)
{
  struct app_model* mdl = NULL;
  enum { CMD_NAME, FORCE_FLAG, MODEL_NAME, ARGC };

  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[FORCE_FLAG]->type == APP_CMDARG_LITERAL
      && argv[MODEL_NAME]->type == APP_CMDARG_STRING);

  APP(get_model(app, CMD_ARGVAL(argv, MODEL_NAME).data.string, &mdl));
  if(mdl == NULL) {
    APP(log(app, APP_LOG_ERROR,
      "the model `%s' does not exist\n",
      CMD_ARGVAL(argv, MODEL_NAME).data.string));
  } else {
    if(CMD_ARGVAL(argv, FORCE_FLAG).is_defined) {
      APP(remove_model(mdl));
    } else {
      bool is_instantiated;
      APP(is_model_instantiated(mdl, &is_instantiated));
      if(is_instantiated == false) {
        APP(remove_model(mdl));
      } else {
        APP(log(app, APP_LOG_ERROR,
          "rm: the model `%s' is still instantiated. Use the --force flag to"
          " remove the model and its instances\n",
          CMD_ARGVAL(argv, MODEL_NAME).data.string));
      }
    }
  }
}

static void
spawn_instance
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv)
{
  enum { CMD_NAME, MODEL_NAME, INSTANCE_NAME, ARGC };
  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[MODEL_NAME]->type == APP_CMDARG_STRING
      && argv[INSTANCE_NAME]->type == APP_CMDARG_STRING);

  if(argv[1]->value_list[0].is_defined == true) {
    struct app_model* mdl = NULL;
    APP(get_model
      (app, CMD_ARGVAL(argv, MODEL_NAME).data.string, &mdl));
    if(mdl == NULL) {
      APP(log(app, APP_LOG_ERROR,
        "the model `%s' does not exist\n",
        CMD_ARGVAL(argv, MODEL_NAME).data.string));
    } else {
      struct app_model_instance* instance = NULL;
      enum app_error app_err = APP_NO_ERROR;

      app_err = app_instantiate_model
        (app,
         mdl,
         CMD_ARGVAL(argv, INSTANCE_NAME).is_defined
          ? CMD_ARGVAL(argv, INSTANCE_NAME).data.string
          : NULL,
         &instance);
      if(app_err != APP_NO_ERROR) {
        APP(log(app, APP_LOG_ERROR,
          "error instantiating the model `%s': %s\n",
          CMD_ARGVAL(argv, MODEL_NAME).data.string,
          app_error_string(app_err)));
      } else {
        struct app_world* world = NULL;
        APP(get_main_world(app, &world));
        app_err = app_world_add_model_instances(world, 1, &instance);
        if(app_err != APP_NO_ERROR) {
          const char* cstr = NULL;
          APP(model_instance_name(instance, &cstr));
          APP(log(app, APP_LOG_ERROR,
            "error adding the instance `%s' to the world: %s\n",
            cstr, app_error_string(app_err)));
          APP(model_instance_ref_put(instance));
        }
      }
    }
  }
}

/*******************************************************************************
 *
 * Objec management command registration functions.
 *
 ******************************************************************************/
enum cmd_error
cmd_setup_object_management_commands(struct app* app)
{
  enum cmd_error cmd_err = CMD_NO_ERROR;
  if(!app) {
    cmd_err= CMD_INVALID_ARGUMENT;
    goto error;
  }

  #define CALL(func) \
    do { \
      const enum app_error app_err = func; \
      if(APP_NO_ERROR != app_err) { \
        cmd_err = app_to_cmd_error(app_err); \
        goto error; \
      } \
    } while(0)

  CALL(app_add_command
    (app, "rename", rename_model, app_model_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING("m", "model", "<model>", NULL, 1, 1, NULL),
      APP_CMDARG_APPEND_STRING(NULL, NULL, "<name>", NULL, 1, 1, NULL),
      APP_CMDARG_END),
     "rename model"));
  CALL(app_add_command
    (app, "rename", rename_instance, app_model_instance_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING("i", "instance", "<instance>", NULL, 1, 1, NULL),
      APP_CMDARG_APPEND_STRING(NULL, NULL, "<name>", NULL, 1, 1, NULL),
      APP_CMDARG_END),
     "rename instance"));

  CALL(app_add_command
    (app, "rm", rm_instance, app_model_instance_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING("i", "instance", "<instance>", NULL, 1, 1, NULL),
      APP_CMDARG_END),
     "remove a model instance"));
  CALL(app_add_command
    (app, "rm", rm_model, app_model_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_LITERAL
        ("f", "force", "remove the model even though it is instantiated", 0, 1),
      APP_CMDARG_APPEND_STRING
        ("m", "model", "<model>", "model to remove", 1, 1, NULL),
      APP_CMDARG_END),
     "remove a model"));

  CALL(app_add_command
    (app, "spawn", spawn_instance, app_model_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING
        ("m", "model", "<model>",
         "name of the model from which an instance is spawned",
         1, 1, NULL),
      APP_CMDARG_APPEND_STRING
        ("n", "name", "<name>",
         "name of the spawned instance",
         0, 1, NULL),
      APP_CMDARG_END),
     "spawn an instance into the world"));

exit:
  return cmd_err;
error:
  if(app)
    CMD(release_object_management_commands(app));
  goto exit;
}

enum cmd_error
cmd_release_object_management_commands(struct app* app)
{
  if(!app)
    return CMD_INVALID_ARGUMENT;

  #define RELEASE_CMD(cmd) \
    do { \
      bool b = false; \
      APP(has_command(app, cmd, &b)); \
      if(b == true) \
        APP(del_command(app, cmd)); \
    } while(0);

  RELEASE_CMD("rename");
  RELEASE_CMD("rm");
  RELEASE_CMD("spawn");

  #undef RELEASE_CMD
  return CMD_NO_ERROR;
}

