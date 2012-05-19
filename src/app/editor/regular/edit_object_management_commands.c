#include "app/editor/regular/edit_context_c.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_object_management_commands.h"
#include "app/editor/edit_context.h"
#include "app/editor/edit_error.h"
#include "app/editor/edit_model_instance_selection.h"
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
   const struct app_cmdarg** argv,
   void* data UNUSED)
{
  struct app_model* mdl = NULL;
  enum { CMD_NAME, OLD_MODEL_NAME, NEW_MODEL_NAME, ARGC };

  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[OLD_MODEL_NAME]->type == APP_CMDARG_STRING
      && argv[NEW_MODEL_NAME]->type == APP_CMDARG_STRING);

  APP(get_model(app, EDIT_CMD_ARGVAL(argv, OLD_MODEL_NAME).data.string, &mdl));
  if(mdl) {
    APP(set_model_name(mdl, EDIT_CMD_ARGVAL(argv, NEW_MODEL_NAME).data.string));
  } else {
    APP(log(app, APP_LOG_ERROR,
      "the model `%s' does not exist\n",
      EDIT_CMD_ARGVAL(argv, OLD_MODEL_NAME).data.string));
  }
}

static void
rename_instance
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data UNUSED)
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
    (app, EDIT_CMD_ARGVAL(argv, OLD_NAME).data.string, &instance));
  if(instance) {
    APP(set_model_instance_name
      (instance, EDIT_CMD_ARGVAL(argv, NEW_NAME).data.string));
  } else {
    APP(log(app, APP_LOG_ERROR,
      "the instance `%s' does not exist\n",
      EDIT_CMD_ARGVAL(argv, OLD_NAME).data.string));
  }
}

static void
rm_instance
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data UNUSED)
{
  struct app_model_instance* instance = NULL;
  enum { CMD_NAME, INSTANCE_NAME, ARGC };

  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[INSTANCE_NAME]->type == APP_CMDARG_STRING);

  APP(get_model_instance
    (app, EDIT_CMD_ARGVAL(argv, INSTANCE_NAME).data.string, &instance));
  if(instance != NULL) {
    APP(remove_model_instance(instance));
  } else {
    APP(log(app, APP_LOG_ERROR,
      "the instance `%s' does not exist\n",
      EDIT_CMD_ARGVAL(argv, INSTANCE_NAME).data.string));
  }
}

static void
rm_model
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data UNUSED)
{
  struct app_model* mdl = NULL;
  enum { CMD_NAME, FORCE_FLAG, MODEL_NAME, ARGC };

  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[FORCE_FLAG]->type == APP_CMDARG_LITERAL
      && argv[MODEL_NAME]->type == APP_CMDARG_STRING);

  APP(get_model(app, EDIT_CMD_ARGVAL(argv, MODEL_NAME).data.string, &mdl));
  if(mdl == NULL) {
    APP(log(app, APP_LOG_ERROR,
      "the model `%s' does not exist\n",
      EDIT_CMD_ARGVAL(argv, MODEL_NAME).data.string));
  } else {
    if(EDIT_CMD_ARGVAL(argv, FORCE_FLAG).is_defined) {
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
          EDIT_CMD_ARGVAL(argv, MODEL_NAME).data.string));
      }
    }
  }
}

static void
select_instance_list
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data)
{
  struct edit_context* ctxt = data;
  size_t i = 0;
  enum { CMD_NAME, INSTANCE_NAME_LIST, ADD_FLAG, ARGC };

  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && data != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[INSTANCE_NAME_LIST]->type == APP_CMDARG_STRING
      && argv[ADD_FLAG]->type == APP_CMDARG_LITERAL);

  if(EDIT_CMD_ARGVAL(argv, ADD_FLAG).is_defined == false) {
    EDIT(clear_model_instance_selection(ctxt->instance_selection));
  }

  for(i = 0;
      i < argv[INSTANCE_NAME_LIST]->count
      && EDIT_CMD_ARGVAL_N(argv, INSTANCE_NAME_LIST, i).is_defined;
      ++i) {
    struct app_model_instance* instance = NULL;
    const char* instance_name = EDIT_CMD_ARGVAL_N
      (argv, INSTANCE_NAME_LIST, i).data.string;
    bool is_already_selected = false;

    APP(get_model_instance(app, instance_name, &instance));
    if(instance == NULL) {
      APP(log(app, APP_LOG_ERROR,
        "the instance `%s' does not exist\n", instance_name));
    } else {
      EDIT(is_model_instance_selected
        (ctxt->instance_selection, instance, &is_already_selected));
      if(is_already_selected == true) {
        EDIT(unselect_model_instance(ctxt->instance_selection, instance));
      } else {
        enum edit_error edit_err = edit_select_model_instance
          (ctxt->instance_selection, instance);
        if(edit_err != EDIT_NO_ERROR) {
          APP(log(app, APP_LOG_ERROR,
            "error selecting the instance `%s'\n", instance_name));
        }
      }
    }
  }
}

static void
spawn_instance
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data UNUSED)
{
  enum { CMD_NAME, MODEL_NAME, INSTANCE_NAME, ARGC };
  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[MODEL_NAME]->type == APP_CMDARG_STRING
      && argv[INSTANCE_NAME]->type == APP_CMDARG_STRING);

  if(EDIT_CMD_ARGVAL(argv, MODEL_NAME).is_defined == true) {
    struct app_model* mdl = NULL;
    APP(get_model
      (app, EDIT_CMD_ARGVAL(argv, MODEL_NAME).data.string, &mdl));
    if(mdl == NULL) {
      APP(log(app, APP_LOG_ERROR,
        "the model `%s' does not exist\n",
        EDIT_CMD_ARGVAL(argv, MODEL_NAME).data.string));
    } else {
      struct app_model_instance* instance = NULL;
      enum app_error app_err = APP_NO_ERROR;

      app_err = app_instantiate_model
        (app,
         mdl,
         EDIT_CMD_ARGVAL(argv, INSTANCE_NAME).is_defined
          ? EDIT_CMD_ARGVAL(argv, INSTANCE_NAME).data.string
          : NULL,
         &instance);
      if(app_err != APP_NO_ERROR) {
        APP(log(app, APP_LOG_ERROR,
          "error instantiating the model `%s': %s\n",
          EDIT_CMD_ARGVAL(argv, MODEL_NAME).data.string,
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
enum edit_error
edit_release_object_management_commands(struct edit_context* ctxt)
{
  if(UNLIKELY(!ctxt))
    return EDIT_INVALID_ARGUMENT;

  #define RELEASE_CMD(cmd) \
    do { \
      bool b = false; \
      APP(has_command(ctxt->app, cmd, &b)); \
      if(b == true) \
        APP(del_command(ctxt->app, cmd)); \
    } while(0);

  RELEASE_CMD("rename");
  RELEASE_CMD("rm");
  RELEASE_CMD("spawn");

  #undef RELEASE_CMD
  return EDIT_NO_ERROR;
}

enum edit_error
edit_setup_object_management_commands(struct edit_context* ctxt)
{
  enum edit_error edit_err = EDIT_NO_ERROR;
  if(UNLIKELY(!ctxt)) {
    edit_err= EDIT_INVALID_ARGUMENT;
    goto error;
  }

  #define CALL(func) \
    do { \
      const enum app_error app_err = func; \
      if(APP_NO_ERROR != app_err) { \
        edit_err = app_to_edit_error(app_err); \
        goto error; \
      } \
    } while(0)

  CALL(app_add_command
    (ctxt->app, "rename", rename_model, NULL, app_model_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING("m", "model", "<model>", NULL, 1, 1, NULL),
      APP_CMDARG_APPEND_STRING(NULL, NULL, "<name>", NULL, 1, 1, NULL),
      APP_CMDARG_END),
     "rename model"));
  CALL(app_add_command
    (ctxt->app, "rename", rename_instance, NULL,
     app_model_instance_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING("i", "instance", "<instance>", NULL, 1, 1, NULL),
      APP_CMDARG_APPEND_STRING(NULL, NULL, "<name>", NULL, 1, 1, NULL),
      APP_CMDARG_END),
     "rename instance"));

  CALL(app_add_command
    (ctxt->app, "rm", rm_instance, NULL, app_model_instance_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING("i", "instance", "<instance>", NULL, 1, 1, NULL),
      APP_CMDARG_END),
     "remove a model instance"));
  CALL(app_add_command
    (ctxt->app, "rm", rm_model, NULL, app_model_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_LITERAL
        ("f", "force", "remove the model even though it is instantiated", 0, 1),
      APP_CMDARG_APPEND_STRING
        ("m", "model", "<model>", "model to remove", 1, 1, NULL),
      APP_CMDARG_END),
     "remove a model"));

  #define CMD_SELECT_MAX_INSTANCE_COUNT 16
  CALL(app_add_command
    (ctxt->app, "select", select_instance_list, ctxt,
     app_model_instance_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING
        ("i", "instance", "<str>", "instance to select",
         0, CMD_SELECT_MAX_INSTANCE_COUNT, NULL),
      APP_CMDARG_APPEND_LITERAL
        ("a", "append",
         "remove/add the selected instances to current selection whether they "
         "are already selected or not", 0, 1),
      APP_CMDARG_END),
     "select up to "STR(CMD_SELECT_MAX_INSTANCE_COUNT)" instances"));
  #undef CMD_SELECT_MAX_INSTANCE_COUNT

  CALL(app_add_command
    (ctxt->app, "spawn", spawn_instance, NULL, app_model_name_completion,
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
  return edit_err;
error:
  if(ctxt)
    EDIT(release_object_management_commands(ctxt));
  goto exit;
}

