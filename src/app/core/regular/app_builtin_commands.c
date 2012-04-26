#include "app/core/regular/app_builtin_commands.h"
#include "app/core/regular/app_c.h"
#include "app/core/regular/app_command_c.h"
#include "app/core/regular/app_model_c.h"
#include "app/core/regular/app_object.h"
#include "app/core/app_command.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "app/core/app_world.h"
#include "renderer/rdr_term.h"
#include "renderer/rdr.h"
#include "stdlib/sl.h"
#include "stdlib/sl_flat_map.h"
#include "stdlib/sl_flat_set.h"
#include "stdlib/sl_hash_table.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
UNUSED static void
complete_string
  (const char* input,
   size_t input_len,
   size_t str_list_len,
   const char* str_list[], /* Must be ordered in ascending order. */
   size_t* start_id,
   size_t* end_id)
{
  size_t begin = 0;
  size_t end = 0;
  bool b = false;
  assert(input && input_len && str_list && str_list_len && start_id && end_id);

  #define LOWER_BOUND 0
  #define UPPER_BOUND 1
  #define DICHOTOMY_SEARCH(bound_type) \
    do { \
      begin = 0; \
      end = str_list_len; \
      b = false; \
      while(begin != end) { \
        const size_t at = begin + (end - begin) / 2; \
        const int cmp = strncmp(input, str_list[at], input_len); \
        if(cmp > 0) { \
          begin = at + 1; \
        } else if(cmp < 0) { \
          end = at; \
        } else { /* cmp == 0. */ \
          if(UPPER_BOUND == bound_type) \
          begin = at + 1; \
          else \
          end = at; \
          b = true; \
        } \
      } \
    } while(0)

  DICHOTOMY_SEARCH(LOWER_BOUND);
  if(false == b) {
    *start_id = *end_id = 0;
  } else {
    *start_id = begin;
    DICHOTOMY_SEARCH(UPPER_BOUND);
    assert(true == b && begin > *start_id);
    *end_id = begin;
  }
  #undef LOWER_BOUND
  #undef UPPER_BOUND
  #undef DICHOTOMY_SEARCH
}

/*******************************************************************************
 *
 * Command functions.
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
cmd_load(struct app* app, size_t argc UNUSED, const struct app_cmdarg** argv)
{
  struct app_model* mdl = NULL;
  enum app_error app_err = APP_NO_ERROR;
  assert(app != NULL
      && argc == 2
      && argv != NULL
      && argv[1]->type == APP_CMDARG_FILE
      && argv[1]->count == 1);

  if(argv[1]->value_list[0].is_defined) {
    app_err = app_create_model(app, argv[1]->value_list[0].data.string, &mdl);
    if(app_err == APP_NO_ERROR) {
      APP_LOG_MSG
        (app->logger,
         "model loaded `%s'\n",
         argv[1]->value_list[0].data.string);
    } else {
      APP_LOG_ERR(app->logger, "model loading error\n");
    }
  } else {
    assert(false);
  }
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
    (app, argv[1]->value_list[0].data.string, app->cmd.stream);
  if(app_err == APP_NO_ERROR) {
    const char* ptr = NULL;
    size_t i = 0;

    rewind(app->cmd.stream);
    do {
      ptr = fgets
        (app->cmd.scratch + i ,
         sizeof(app->cmd.scratch)/sizeof(char) - i - 1, /* -1 <=> '\0' */
         app->cmd.stream);
      if(ptr)
        i += strlen(ptr);
    } while(ptr);
    APP_LOG_MSG(app->logger, "%s", app->cmd.scratch);
  }
}

static void
cmd_ls(struct app* app, size_t argc UNUSED, const struct app_cmdarg** argv)
{
  size_t len = 0;
  size_t i = 0;
  bool new_line = false;

  assert(app != NULL
      && argc == 4
      && argv != NULL
      && argv[1]->type == APP_CMDARG_LITERAL
      && argv[2]->type == APP_CMDARG_LITERAL
      && argv[3]->type == APP_CMDARG_LITERAL);

  if(argv[1]->value_list[0].is_defined) { /* commands */
    const char** name_list = NULL;
    SL(flat_set_buffer
      (app->cmd.name_set, &len, NULL, NULL, (void**)&name_list));
    for(i = 0; i < len; ++i) {
      APP_LOG_MSG(app->logger, "%s\n", name_list[i]);
    }
    APP_LOG_MSG(app->logger, "[total %zu commands]\n", len);
    new_line = true;
  }
  if(argv[3]->value_list[0].is_defined) { /* models */
    struct app_model** model_list = NULL;
    APP(get_model_list(app, &len, &model_list));
    if(new_line)
      APP_LOG_MSG(app->logger, "\n");
    for(i = 0; i < len; ++i) {
      const char* name = NULL;
      APP(model_name(model_list[i], &name));
      APP_LOG_MSG(app->logger, "%s\n", name);
    }
    APP_LOG_MSG(app->logger, "[total %zu models]\n", len);
    new_line = true;
  }
  if(argv[2]->value_list[0].is_defined) { /* model-instances */
    struct app_model_instance** instance_list = NULL;
    APP(get_model_instance_list(app, &len,&instance_list));
    if(new_line)
      APP_LOG_MSG(app->logger, "\n");
    for(i = 0; i < len; ++i) {
      const char* name = NULL;
      APP(model_instance_name(instance_list[i], &name));
      APP_LOG_MSG(app->logger, "%s\n", name);
    }
    APP_LOG_MSG(app->logger, "[total %zu model instances]\n", len);
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
cmd_spawn(struct app* app, size_t argc UNUSED, const struct app_cmdarg** argv)
{
  assert(app != NULL
      && argc == 2
      && argv != NULL
      && argv[1]->type == APP_CMDARG_STRING);

  if(argv[1]->value_list[0].is_defined == true) {
    struct app_object* obj = NULL;
    APP(get_object
      (app, APP_MODEL, argv[1]->value_list[0].data.string, &obj));
    if(obj == NULL) {
      APP_LOG_ERR
        (app->logger,
         "the model `%s' does not exist\n",
         argv[1]->value_list[0].data.string);
    } else {
      struct app_model* mdl = app_object_to_model(obj);
      struct app_model_instance* instance = NULL;
      enum app_error app_err = APP_NO_ERROR;

      app_err = app_instantiate_model(app, mdl, NULL, &instance);
      if(app_err != APP_NO_ERROR) {
        APP_LOG_ERR
          (app->logger,
           "error instantiating the model `%s': %s\n",
           argv[1]->value_list[0].data.string,
           app_error_string(app_err));
      } else {
        app_err = app_world_add_model_instances(app->world, 1, &instance);
        if(app_err != APP_NO_ERROR) {
          const char* cstr = NULL;
          APP(model_instance_name(instance, &cstr));
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
    (app, "help", cmd_help, app_command_name_completion, APP_CMDARGV
     (APP_CMDARG_APPEND_STRING(NULL, NULL, "<command>", NULL, 1, 1, NULL),
      APP_CMDARG_END),
     "give command informations"));
  CALL(app_add_command
    (app, "ls", cmd_ls, NULL, APP_CMDARGV
     (APP_CMDARG_APPEND_LITERAL
      ("c", "commands", "list the registered commands", 0, 1),
      APP_CMDARG_APPEND_LITERAL
      ("i", "model-instances", "list the spawned model instances", 0, 1),
      APP_CMDARG_APPEND_LITERAL
      ("m", "models", "list the loaded models", 0, 1),
      APP_CMDARG_END),
     "list application contents"));
  CALL(app_add_command
    (app, "load", cmd_load, NULL, APP_CMDARGV
     (APP_CMDARG_APPEND_FILE("m", "model", "<path>", NULL, 1, 1),
      APP_CMDARG_END),
     "load resources"));
  CALL(app_add_command
    (app, "spawn", cmd_spawn, app_model_name_completion, APP_CMDARGV
     (APP_CMDARG_APPEND_STRING("m", "model", "<model-name>", NULL, 0, 1, NULL),
      APP_CMDARG_END),
     "spawn an instance into the world"));
  #undef CALL

exit:
  return app_err;
error:
  goto exit;
}

