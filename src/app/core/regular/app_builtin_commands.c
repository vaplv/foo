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
static void
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

static void
setup_value_list
  (struct app_cmdarg_value_list* list,
   const char** str_list,
   size_t str_list_len,
   const char* input,
   size_t input_len)
{
  assert(list && (!str_list_len || str_list));

  #ifndef NDEBUG
  {
    size_t i = 0;
    for(i = 0; i < str_list_len; ++i) {
      /* We assume that the str list is ordered in ascending order. */
      if(i > 0)
        assert(strcmp(str_list[i-1], str_list[i]) < 0);
    }
  }
  #endif
  if(str_list) {
    if(input == NULL) {
      list->buffer = str_list;
      list->length = str_list_len;
    } else {
      size_t start, end;
      complete_string(input, input_len, str_list_len, str_list, &start, &end);
      list->buffer = str_list + start;
      list->length = end - start;
    }
  }
}

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

static struct app_cmdarg_value_list
cmd_load_options(struct app* app UNUSED, const char* input, size_t input_len)
{
  static const char* options[] = { "--model" };
  struct app_cmdarg_value_list value_list;
  memset(&value_list, 0, sizeof(value_list));

  setup_value_list
    (&value_list,
     options,
     sizeof(options)/sizeof(const char*),
     input,
     input_len);
  return value_list;
}

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

static struct app_cmdarg_value_list
cmd_ls_options(struct app* app UNUSED, const char* input, size_t input_len)
{
  static const char* options[] = {
      "--all",
      "--commands",
      "--model-instances",
      "--models"
  };
  struct app_cmdarg_value_list value_list;
  memset(&value_list, 0, sizeof(value_list));
  setup_value_list
    (&value_list,
     options,
     sizeof(options)/sizeof(const char*),
     input,
     input_len);
  return value_list;
}

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
    APP(get_model_list(app, &len, &model_list));
    for(i = 0; i < len; ++i) {
      const char* name = NULL;
      APP(get_model_name(model_list[i], &name));
      APP_LOG_MSG(app->logger, "%s\n", name);
    }
    APP_LOG_MSG(app->logger, "[total %zu models]%s", len, all ? "\n\n":"\n");
  }
  if(0 == strcmp("--model-instances", argv[1].value.string) || all) {
    struct app_model_instance** instance_list = NULL;
    APP(get_model_instance_list(app, &len,&instance_list));
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

static struct app_cmdarg_value_list
cmd_model_list(struct app* app, const char* input, size_t input_len)
{
  struct app_cmdarg_value_list value_list;
  const char** model_list = NULL;
  size_t len = 0;
  memset(&value_list, 0, sizeof(value_list));

  SL(flat_map_key_buffer
    (app->object_map[APP_MODEL], &len, NULL, NULL, (void**)&model_list));
  setup_value_list
    (&value_list,
     model_list,
     len,
     input,
     input_len);
  return value_list;
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
     1, APP_CMDARGV(APP_CMDARG_APPEND_STRING(cmd_model_list)),
     "spawn - spawn a instance of MODEL_NAME into the world\n"
     "Usage: spawn MODEL_NAME\n"));

  #undef CALL

exit:
  return app_err;
error:
  goto exit;
}

