#include "app/core/regular/app_builtin_commands.h"
#include "app/core/regular/app_c.h"
#include "app/core/app_command.h"
#include "app/core/app_cvar.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "renderer/rdr_term.h"
#include "renderer/rdr.h"
#include "stdlib/sl.h"
#include "stdlib/sl_flat_map.h"
#include "stdlib/sl_flat_set.h"
#include "sys/sys.h"
#include <float.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define ARGVAL(argv, i) (argv)[i]->value_list[0]

/*******************************************************************************
 *
 * Miscellaneous command functions.
 *
 ******************************************************************************/
static void
cmd_exit
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv UNUSED)
{
  app->post_exit = true;
}

static void
cmd_help(struct app* app, size_t argc UNUSED, const struct app_cmdarg** argv)
{
  enum app_error app_err = APP_NO_ERROR;
  assert(app != NULL
      && argc == 2
      && argv != NULL
      && argv[1]->type == APP_CMDARG_STRING
      && argv[1]->value_list[0].is_defined == true);

  rewind(app->cmd.stream);
  app_err = app_man_command
    (app,
     argv[1]->value_list[0].data.string,
     NULL,
     sizeof(app->cmd.scratch)/sizeof(char),
     app->cmd.scratch);
  if(app_err == APP_NO_ERROR)
    APP_PRINT_MSG(app->logger, "%s", app->cmd.scratch);
}

static void
cmd_ls(struct app* app, size_t argc UNUSED, const struct app_cmdarg** argv)
{
  bool new_line = false;
  size_t len = 0;

  assert(app != NULL
      && argc == 4
      && argv != NULL
      && argv[1]->type == APP_CMDARG_LITERAL
      && argv[2]->type == APP_CMDARG_LITERAL
      && argv[3]->type == APP_CMDARG_LITERAL);

  if(argv[1]->value_list[0].is_defined) { /* commands */
    const char** name_list = NULL;
    size_t i = 0;
    SL(flat_set_buffer
      (app->cmd.name_set, &len, NULL, NULL, (void**)&name_list));
    for(i = 0; i < len; ++i) {
      APP_PRINT_MSG(app->logger, "%s\n", name_list[i]);
    }
    APP_PRINT_MSG(app->logger, "[total %zu commands]\n", len);
    new_line = true;
  }
  if(argv[3]->value_list[0].is_defined) { /* models */
    struct app_model_it model_it;
    bool is_end_reached = false;
    memset(&model_it, 0, sizeof(model_it));

    if(new_line)
      APP_PRINT_MSG(app->logger, "\n");

    APP(get_model_list_begin(app, &model_it, &is_end_reached));
    len = 0;
    while(is_end_reached == false) {
      const char* name = NULL;
      APP(model_name(model_it.model, &name));
      APP_PRINT_MSG(app->logger, "%s\n", name);
      APP(model_it_next(&model_it, &is_end_reached));
      ++len;
    }
    APP_PRINT_MSG(app->logger, "[total %zu models]\n", len);
    new_line = true;
  }
  if(argv[2]->value_list[0].is_defined) { /* model-instances */
    struct app_model_instance_it instance_it;
    bool is_end_reached = false;
    memset(&instance_it, 0, sizeof(instance_it));

    if(new_line)
      APP_PRINT_MSG(app->logger, "\n");

    APP(get_model_instance_list_begin(app, &instance_it, &is_end_reached));
    len = 0;
    while(is_end_reached == false) {
      const char* name = NULL;
      APP(model_instance_name(instance_it.instance, &name));
      APP_PRINT_MSG(app->logger, "%s\n", name);
      APP(model_instance_it_next(&instance_it, &is_end_reached));
      ++len;
    }
    APP_PRINT_MSG(app->logger, "[total %zu model instances]\n", len);
  }
}

static void
cmd_clear
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv UNUSED)
{
  assert(app != NULL);
  RDR(clear_term(app->term.render_term, RDR_TERM_STDOUT));
}

static void
cmd_set
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv)
{
  const struct app_cvar* cvar = NULL;
  const char* cvar_name = NULL;
  const char* cvar_value = NULL;
  long int i = 0;
  float f = 0.f;
  char* ptr = NULL;
  enum { CMD_NAME, CVAR_NAME, CVAR_VALUE, ARGC };

  assert(app != NULL
      && argc == ARGC
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[CVAR_NAME]->type == APP_CMDARG_STRING
      && argv[CVAR_VALUE]->type == APP_CMDARG_STRING);

  cvar_name = ARGVAL(argv, CVAR_NAME).data.string;
  cvar_value = ARGVAL(argv, CVAR_VALUE).data.string;

  APP(get_cvar(app, cvar_name, &cvar));
  if(cvar == NULL) {
    APP_PRINT_ERR
      (app->logger,
       "%s: cvar not found `%s'\n",
       ARGVAL(argv, CMD_NAME).data.string,
       cvar_name);
  } else {
    if(ARGVAL(argv, CVAR_VALUE).is_defined == false) {
      switch(cvar->type) {
        case APP_CVAR_BOOL:
          APP_PRINT_MSG(app->logger, "%s %d\n", cvar_name, cvar->value.boolean);
          break;
        case APP_CVAR_INT:
          APP_PRINT_MSG(app->logger, "%s %d\n", cvar_name, cvar->value.integer);
          break;
        case APP_CVAR_FLOAT:
          APP_PRINT_MSG(app->logger, "%s %f\n", cvar_name, cvar->value.real);
          break;
        case APP_CVAR_STRING:
          APP_PRINT_MSG(app->logger, "%s %s\n", cvar_name, cvar->value.string);
          break;
        default: assert(0); break;
      }
    } else {
      #define ERR_MSG(msg) \
        APP_PRINT_ERR \
        (app->logger, "%s %s: " msg " `%s'\n", \
         ARGVAL(argv, CMD_NAME).data.string, \
         ARGVAL(argv, CVAR_NAME).data.string, \
         ARGVAL(argv, CVAR_VALUE).data.string)

      switch(cvar->type) {
        case APP_CVAR_BOOL:
          i = strtol(cvar_value, &ptr, 10);
          if(*ptr != '\0') {
            ERR_MSG("unexpected non integer value");
          } else {
            APP(set_cvar(app, cvar_name, APP_CVAR_BOOL_VALUE(i != 0L)));
          }
          break;
        case APP_CVAR_INT:
          i = strtol(cvar_value, &ptr, 10);
          if(*ptr != '\0') {
            ERR_MSG("unexpected non integer value");
          } else {
            if(i > INT_MAX || i < INT_MIN) {
              ERR_MSG("out of range integer value");
            } else {
              APP(set_cvar(app, cvar_name, APP_CVAR_INT_VALUE(i)));
            }
          }
          break;
        case APP_CVAR_FLOAT:
          f = strtof(cvar_value, &ptr);
          if(*ptr != '\0') {
            ERR_MSG("unexpected non real value");
          } else {
            APP(set_cvar(app, cvar_name, APP_CVAR_FLOAT_VALUE(f)));
          }
          break;
        case APP_CVAR_STRING:
          APP(set_cvar(app, cvar_name, APP_CVAR_STRING_VALUE(cvar_value)));
          break;
        default: assert(0); break;
      }
      #undef ERR_MSG
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
    (app, "clear", cmd_clear, NULL, NULL, "clear the terminal screen"));

  CALL(app_add_command
    (app, "exit", cmd_exit, NULL, NULL, "cause the application to exit"));

  CALL(app_add_command
    (app, "help", cmd_help, app_command_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING(NULL, NULL, "<command>", NULL, 1, 1, NULL),
      APP_CMDARG_END),
     "give command informations"));

  CALL(app_add_command
    (app, "ls", cmd_ls, NULL,
     APP_CMDARGV
     (APP_CMDARG_APPEND_LITERAL
      ("c", "commands", "list the registered commands", 0, 1),
      APP_CMDARG_APPEND_LITERAL
      ("i", "model-instances", "list the spawned model instances", 0, 1),
      APP_CMDARG_APPEND_LITERAL
      ("m", "models", "list the loaded models", 0, 1),
      APP_CMDARG_END),
     "list application contents"));

  CALL(app_add_command
    (app, "set", cmd_set, app_cvar_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING(NULL, NULL, "<cvar>",NULL, 1, 1, NULL),
      APP_CMDARG_APPEND_STRING(NULL, NULL, "<value>", NULL, 0, 1, NULL),
      APP_CMDARG_END),
     "set a client variable"));

  #undef CALL

exit:
  return app_err;
error:
  goto exit;
}

#undef ARGVAL

