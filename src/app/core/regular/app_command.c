#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_c.h"
#include "app/core/regular/app_command_c.h"
#include "app/core/app_command.h"
#include "stdlib/sl.h"
#include "stdlib/sl_hash_table.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct app_command {
  void(*func)(struct app*, size_t, struct app_cmdarg*);
  size_t argc;
  struct app_cmdarg_desc arg_desc_list[];
};

/*******************************************************************************
 *
 * Hash table functions.
 *
 ******************************************************************************/
static size_t
hash_str(const void* key)
{
  const char* str = *(const char**)key;
  return sl_hash(str, strlen(str));
}

static bool
eq_str(const void* key0, const void* key1)
{
  const char* str0 = *(const char**)key0;
  const char* str1 = *(const char**)key1;
  return strcmp(str0, str1) == 0;
}

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static enum app_error
token_to_cmdarg
  (struct app* app UNUSED,
   const char* tkn,
   struct app_cmdarg_desc* desc,
   struct app_cmdarg* arg)
{
  char* endptr = NULL;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;
  assert(app && tkn && desc && arg);

  arg->type = desc->type;
  switch(desc->type) {
    case APP_CMDARG_INT:
      arg->value.integer =
        (int)MIN(MAX(strtol(tkn, &endptr, 10), INT_MIN), INT_MAX);
      if(desc->domain.integer.min <= desc->domain.integer.max) {
        arg->value.integer =
          MAX(arg->value.integer, desc->domain.integer.min);
        arg->value.integer =
          MIN(arg->value.integer, desc->domain.integer.max);
      }
      break;
    case APP_CMDARG_FLOAT:
      arg->value.real = strtof(tkn, &endptr);
      endptr += *endptr == 'f'; /* Float suffix. */
      if(desc->domain.real.min <= desc->domain.real.max) {
        arg->value.real = MAX(arg->value.real, desc->domain.real.min);
        arg->value.real = MIN(arg->value.real, desc->domain.real.max);
      }
      break;
    case APP_CMDARG_STRING:
      arg->value.string = tkn;
      if(desc->domain.string.value_list) {
        for(i = 0; desc->domain.string.value_list[i]; ++i) {
          if(0 == strcmp(desc->domain.string.value_list[i], tkn))
            break;
        }
        if(NULL == desc->domain.string.value_list[i]) {
          arg->value.string = NULL;
          endptr = "error";
        }
      }
      break;
    default:
      assert(false);
      break;
  }
  if(endptr && *endptr != '\0') {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
exit:
  return app_err;
error:
  goto exit;
}

static void
del_all_commands(struct app* app)
{
  struct sl_hash_table_it it;
  bool b = false;
  assert(app && app->cmd.htbl);

  SL(hash_table_begin(app->cmd.htbl, &it, &b));
  while(!b) {
    MEM_FREE(app->allocator, (*(struct app_command**)it.pair.data));
    MEM_FREE(app->allocator, (*(char**)it.pair.key));
    SL(hash_table_it_next(&it, &b));
  }
  SL(hash_table_clear(app->cmd.htbl));
}

/*******************************************************************************
 *
 * Command functions.
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_add_command
  (struct app* app,
   const char* name,
   void (*func)(struct app*, size_t argc, struct app_cmdarg* argv),
   size_t argc,
   const struct app_cmdarg_desc arg_desc_list[])
{
  void* ptr = NULL;
  struct app_command* cmd = NULL;
  char* cmd_name = NULL;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app
  || !name
  || !func
  || argc > APP_MAX_CMDARGS
  || (argc && !arg_desc_list)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(hash_table_find(app->cmd.htbl, &name, &ptr));
  if(ptr) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  for(i = 0; i < argc; ++i) {
    if(arg_desc_list[i].type == APP_NB_CMDARG_TYPES) {
      app_err = APP_INVALID_ARGUMENT;
      goto error;
    }
  }
  cmd = MEM_CALLOC
    (app->allocator,
     1,
     sizeof(struct app_command) + argc*sizeof(struct app_cmdarg_desc));
  if(!cmd) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  cmd->func = func;
  cmd->argc = argc;
  if(argc) {
    memcpy
      (cmd->arg_desc_list,
       arg_desc_list,
       sizeof(struct app_cmdarg_desc)*argc);
  }
  cmd_name = MEM_CALLOC(app->allocator, strlen(name) + 1, sizeof(char));
  strcpy(cmd_name, name);

  assert(app->cmd.htbl);
  sl_err = sl_hash_table_insert(app->cmd.htbl, &cmd_name, &cmd);
  if(SL_NO_ERROR != sl_err) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
exit:
  return app_err;
error:
  /* The command insertion is the last action of this function. The command is
   * thus inserted only if no error occurs and consequently we don't have to
   * manage its deletion from the command system when an error is detected. */
  if(cmd)
    MEM_FREE(app->allocator, cmd);
  if(cmd_name)
    MEM_FREE(app->allocator, cmd_name);
  goto exit;
}

EXPORT_SYM enum app_error
app_del_command(struct app* app, const char* name)
{
  struct sl_pair pair;
  struct app_command* cmd = NULL;
  char* cmd_name = NULL;
  enum app_error app_err = APP_NO_ERROR;
  size_t nb_erased = 0;

  if(!app || !name) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(hash_table_find_pair(app->cmd.htbl, &name, &pair));
  if(!SL_IS_PAIR_VALID(&pair)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  cmd_name = *(char**)pair.key;
  cmd = *(struct app_command**)pair.data;
  SL(hash_table_erase(app->cmd.htbl, &name, &nb_erased));
  assert(nb_erased == 1);
  MEM_FREE(app->allocator, cmd_name);
  MEM_FREE(app->allocator, cmd);
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_execute_command(struct app* app, const char* command)
{
  struct app_cmdarg argv[APP_MAX_CMDARGS];
  struct app_command** cmd = NULL;
  char* name = NULL;
  char* tkn = NULL;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;

  if(!app || !command) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  if(strlen(command) + 1 > sizeof(app->cmd.scratch) / sizeof(char)) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  /* Copy the command into the mutable scratch buffer of the command system. */
  strcpy(app->cmd.scratch, command);

  /* Retrieve the first token <=> command name. */
  name = strtok(app->cmd.scratch, " \t");
  SL(hash_table_find(app->cmd.htbl, &name, (void**)&cmd));
  if(!cmd) {
    APP_LOG_ERR(app->logger, "%s: command not found\n", name);
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  for(i = 0; i < (*cmd)->argc; ++i) {
    tkn = strtok(NULL, " \t");
    if(!tkn) {
      APP_LOG_ERR(app->logger, "%s: missing operand\n", name);
      app_err = APP_INVALID_ARGUMENT;
      goto error;
    }
    app_err = token_to_cmdarg(app, tkn, (*cmd)->arg_desc_list + i, argv + i);
    if(APP_NO_ERROR != app_err) {
      APP_LOG_ERR(app->logger, "%s: bad argument `%s'\n", name, tkn);
      goto error;
    }
  }
  (*cmd)->func(app, (*cmd)->argc, argv);
exit:
  return app_err;
error:
  goto exit;
}

/*******************************************************************************
 *
 * Private functions.
 *
 ******************************************************************************/
enum app_error
app_init_command_system(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_create_hash_table
    (sizeof(char*),
     ALIGNOF(char*),
     sizeof(struct app_command*),
     ALIGNOF(struct app_command*),
     hash_str,
     eq_str,
     app->allocator,
     &app->cmd.htbl);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
exit:
  return app_err;
error:
  APP(shutdown_command_system(app));
  goto exit;
}

enum app_error
app_shutdown_command_system(struct app* app)
{
  if(!app)
    return APP_INVALID_ARGUMENT;

  if(app->cmd.htbl) {
    del_all_commands(app);
    SL(free_hash_table(app->cmd.htbl));
    app->cmd.htbl = NULL;
  }
  return APP_NO_ERROR;
}

