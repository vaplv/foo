#include "app/editor/regular/edit_context_c.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_load_save_commands.h"
#include "app/editor/edit_context.h"
#include "app/editor/edit_error.h"
#include "app/core/app.h"
#include "app/core/app_command.h"
#include "app/core/app_cvar.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "maths/simd/aosf44.h"
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
save_map(struct app* app, const char* cmd_name, const char* output_file)
{
  FILE* file = NULL;
  struct app_model_it model_it;
  struct app_model_instance_it instance_it;
  bool is_end_reached = false;
  assert(app && cmd_name && output_file);
  memset(&model_it, 0, sizeof(model_it));
  memset(&instance_it, 0, sizeof(instance_it));

  file = fopen(output_file, "w");
  if(file == NULL) {
    APP(log(app, APP_LOG_ERROR,
      "%s: error opening output file `%s'\n", cmd_name, output_file));
    goto error;
  }

  #define FPRINTF(file, str, ...) \
    do { \
      if(fprintf(file, str, __VA_ARGS__) < 0) { \
        APP(log(app, APP_LOG_ERROR, \
          "%s: error writing file `%s'\n", cmd_name, output_file)); \
        goto error; \
      } \
    } while(0)

  /* Save the model list. */
  APP(get_model_list_begin(app, &model_it, &is_end_reached));
  while(is_end_reached == false) {
    const char* path = NULL;
    const char* name = NULL;
    APP(model_path(model_it.model, &path));
    APP(model_name(model_it.model, &name));
    FPRINTF(file, "load -m %s -n %s\n", path, name);
    APP(model_it_next(&model_it, &is_end_reached));
  }

  /* Save the instance list. */
  APP(get_model_instance_list_begin(app, &instance_it, &is_end_reached));
  while(is_end_reached == false) {
    struct app_model* model = NULL;
    const char* mdl_name = NULL;
    const char* inst_name = NULL;
    const struct aosf44* transform = NULL;
    ALIGN(16) float tmp[16];

    APP(model_instance_get_model(instance_it.instance, &model));
    APP(model_instance_name(instance_it.instance, &inst_name));
    APP(model_name(model, &mdl_name));
    FPRINTF(file, "spawn -m %s -n %s\n", mdl_name, inst_name);

    APP(get_raw_model_instance_transform(instance_it.instance, &transform));
    aosf44_store(tmp, transform);
    FPRINTF
      (file,
       "transform -i %s "
       "--e00 %.8f --e10 %.8f --e20 %.8f --e30 %.8f "
       "--e01 %.8f --e11 %.8f --e21 %.8f --e31 %.8f "
       "--e02 %.8f --e12 %.8f --e22 %.8f --e32 %.8f "
       "--e03 %.8f --e13 %.8f --e23 %.8f --e33 %.8f\n",
       inst_name,
       tmp[0], tmp[1], tmp[2], tmp[3], /* column 0 */
       tmp[4], tmp[5], tmp[6], tmp[7], /* column 1 */
       tmp[8], tmp[9], tmp[10], tmp[11], /* column 2 */
       tmp[12], tmp[13], tmp[14], tmp[15]); /* column 3 */
    APP(model_instance_it_next(&instance_it, &is_end_reached));
  }
  #undef FPRINTF
  APP(log(app, APP_LOG_INFO,
    "%s: map saved `%s'\n", cmd_name, output_file));
exit:
  if(file) {
    fflush(file);
    if(fclose(file) != 0) {
      APP(log(app, APP_LOG_ERROR,
        "%s: error closing output file `%s'\n", cmd_name, output_file));
    }
  }
  return;
error:
  goto exit;
}

/*******************************************************************************
 *
 * Command functions.
 *
 ******************************************************************************/
static void
load_model
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data UNUSED)
{
  struct app_model* mdl = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum { CMD_NAME, FILE_NAME, MODEL_NAME, ARGC };

  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[FILE_NAME]->type == APP_CMDARG_FILE
      && argv[MODEL_NAME]->type == APP_CMDARG_STRING);

  app_err = app_create_model
    (app,
     EDIT_CMD_ARGVAL(argv, FILE_NAME).data.string,
     EDIT_CMD_ARGVAL(argv, MODEL_NAME).is_defined == true
      ? EDIT_CMD_ARGVAL(argv, MODEL_NAME).data.string
      : NULL,
     &mdl);
  if(app_err != APP_NO_ERROR) {
    APP(log(app, APP_LOG_ERROR, "model loading error\n"));
  } else {
    APP(log(app, APP_LOG_INFO,
      "model loaded `%s'\n", EDIT_CMD_ARGVAL(argv, FILE_NAME).data.string));
  }
}

static void
load_map
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data UNUSED)
{
  #define MAXLEN 2048
  char buffer[MAXLEN];
  char filename[256];
  char cmdname[64];
  FILE* file = NULL;
  char* ptr = NULL;
  enum app_error app_err = APP_NO_ERROR;

  enum { CMD_NAME, FILE_NAME, ARGC };

  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[FILE_NAME]->type == APP_CMDARG_FILE);

  /* We have to copy the arg since they mey be invalidate by the execution of
   * new commands. */
  if(strlen(EDIT_CMD_ARGVAL(argv, FILE_NAME).data.string) + 1
   > sizeof(filename)/sizeof(char)) {
    strcpy(filename, "<none>");
  } else {
    strcpy(filename, EDIT_CMD_ARGVAL(argv, FILE_NAME).data.string);
  }
  if(strlen(EDIT_CMD_ARGVAL(argv, CMD_NAME).data.string) + 1
   > sizeof(cmdname)/sizeof(char)) {
    strcpy(cmdname, "<none>");
  } else {
    strcpy(cmdname, EDIT_CMD_ARGVAL(argv, CMD_NAME).data.string);
  }

  file = fopen(filename, "r");
  if(!file) {
    APP(log(app, APP_LOG_ERROR,
      "%s: error opening file `%s'\n", cmdname, filename));
    goto error;
  }

  app_err = app_set_cvar
    (app, "edit_project_path", APP_CVAR_STRING_VALUE(filename));
  if(app_err != APP_NO_ERROR) {
    APP(log(app, APP_LOG_ERROR,
      "%s: error setting the `edit_project_path' value: %s",
      filename, app_error_string(app_err)));
    goto error;
  }
  APP(cleanup(app));
  while(NULL != (ptr = fgets(buffer, MAXLEN, file))) {
    size_t len = strlen(ptr);
    if(len) {
      /* Remove the "\n" char */
      if(ptr[len - 1] == '\n')
        ptr[len - 1] = '\0';

      app_err = app_execute_command(app, ptr);
      if(app_err != APP_NO_ERROR) {
        APP(log(app, APP_LOG_ERROR,
        "%s: error loading file `%s': %s\n",
        cmdname, filename, app_error_string(app_err)));
        goto error;
      }
    }
  }
  APP(log(app, APP_LOG_INFO, "%s: map loaded `%s'\n", cmdname, filename));

exit:
  if(file) {
    if(fclose(file) != 0) {
      APP(log(app, APP_LOG_ERROR,
        "%s: error closing map file `%s'\n",
        EDIT_CMD_ARGVAL(argv, CMD_NAME).data.string,
        EDIT_CMD_ARGVAL(argv, FILE_NAME).data.string));
    }
  }
  return;
error:
  goto exit;
}

/* Basic save command. */
static void
save
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data)
{
  struct edit_context* ctxt = data;
  const char* cmd_name = NULL;
  const char* output_file = NULL;
  FILE* file = NULL;
  int err = 0;
  enum { CMD_NAME, OUTPUT_FILE, FORCE_FLAG, ARGC };

  assert(app != NULL
      && data != NULL
      && argc == ARGC
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[OUTPUT_FILE]->type == APP_CMDARG_FILE
      && argv[FORCE_FLAG]->type == APP_CMDARG_LITERAL);

  cmd_name = EDIT_CMD_ARGVAL(argv, CMD_NAME).data.string;

  /* Define the output path. */
  if(EDIT_CMD_ARGVAL(argv, OUTPUT_FILE).is_defined == false) {
    if(ctxt->cvars.project_path->value.string == NULL) {
      APP(log(app, APP_LOG_ERROR,
        "%s: undefined output project path\n", cmd_name));
    } else {
      output_file = ctxt->cvars.project_path->value.string;
    }
  } else {
    output_file = EDIT_CMD_ARGVAL(argv, OUTPUT_FILE).data.string;

    APP(set_cvar
      (app, "edit_project_path", APP_CVAR_STRING_VALUE(output_file)));
    output_file = ctxt->cvars.project_path->value.string;

    file = fopen(output_file, "r");
    if(file!=NULL && EDIT_CMD_ARGVAL(argv, FORCE_FLAG).is_defined == false) {
      APP(log(app, APP_LOG_ERROR,
        "%s: the file `%s' already exist. "
        "Use the --force flag to overwrite it\n",
        cmd_name,
        output_file));
      output_file = NULL;
      err = fclose(file);
      if(err != 0) {
        APP(log(app, APP_LOG_ERROR,
          "%s: unexpected error closing the existing file `%s'\n",
          cmd_name, output_file));
      }
    }
  }

  /* Save the map. */
  if(output_file) {
    save_map(app, cmd_name, output_file);
  }
}

/*******************************************************************************
 *
 * Load save command registration functions.
 *
 ******************************************************************************/
enum edit_error
edit_release_load_save_commands(struct edit_context* ctxt)
{
  if(!ctxt)
    return EDIT_INVALID_ARGUMENT;

  #define RELEASE_CMD(cmd) \
    do { \
      bool b = false; \
      APP(has_command(ctxt->app, cmd, &b)); \
      if(b == true) \
        APP(del_command(ctxt->app, cmd)); \
    } while(0);

  RELEASE_CMD("load");
  RELEASE_CMD("save");

  #undef RELEASE_CMD
  return EDIT_NO_ERROR;
}

enum edit_error
edit_setup_load_save_commands(struct edit_context* ctxt)
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
    (ctxt->app, "load", load_model, NULL, NULL,
     APP_CMDARGV
     (APP_CMDARG_APPEND_FILE("m", "model", "<path>", NULL, 1, 1),
      APP_CMDARG_APPEND_STRING("n", "name", "<name>", NULL, 0, 1, NULL),
      APP_CMDARG_END),
     "load a model"));
  CALL(app_add_command
    (ctxt->app, "load", load_map, NULL, NULL,
     APP_CMDARGV
     (APP_CMDARG_APPEND_FILE("M", "map", "<map>", NULL, 1, 1),
      APP_CMDARG_END),
     "load a map"));

  CALL(app_add_command
    (ctxt->app, "save", save, ctxt, NULL,
     APP_CMDARGV
     (APP_CMDARG_APPEND_FILE
        ("o", "output", "<path>", "output file", 0, 1),
      APP_CMDARG_APPEND_LITERAL
        ("f", "force",
         "force save the file even though it already exists",
         0, 1),
      APP_CMDARG_END),
     "save the main world"));

  #undef CALL

exit:
  return edit_err;
error:
  if(ctxt)
    EDIT(release_load_save_commands(ctxt));
  goto exit;
}


