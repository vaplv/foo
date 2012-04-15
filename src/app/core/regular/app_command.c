#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_c.h"
#include "app/core/regular/app_command_c.h"
#include "app/core/regular/app_builtin_commands.h"
#include "app/core/app_command.h"
#include "app/core/app_command_buffer.h"
#include "stdlib/sl.h"
#include "stdlib/sl_flat_set.h"
#include "stdlib/sl_hash_table.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 *
 * Hash table/set functions.
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

static int
cmp_str(const void* a, const void* b)
{
  const char* str0 = *(const char**)a;
  const char* str1 = *(const char**)b;
  return strcmp(str0, str1);
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
    app_err = APP_COMMAND_ERROR;
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
    struct app_command* cmd = *(struct app_command**)it.pair.data;
    if(cmd->description)
      MEM_FREE(app->allocator, cmd->description);
    MEM_FREE(app->allocator, cmd);
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
   const struct app_cmdarg_desc arg_desc_list[],
   const char* description)
{
  void* ptr = NULL;
  struct app_command* cmd = NULL;
  char* cmd_name = NULL;
  size_t i = 0;
  const size_t adjusted_argc = argc + 1; /* +1 <=> command name. */
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
     sizeof(struct app_command) + adjusted_argc*sizeof(struct app_cmdarg_desc));
  if(!cmd) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  if(NULL == description) {
    cmd->description = NULL;
  } else {
    cmd->description = MEM_CALLOC
      (app->allocator, strlen(description) + 1, sizeof(char));
    if(!cmd->description) {
      app_err = APP_MEMORY_ERROR;
      goto error;
    }
    strcpy(cmd->description, description);
  }
  cmd->func = func;
  cmd->argc = adjusted_argc;
  cmd->arg_desc_list[0].type = APP_CMDARG_STRING;
  cmd->arg_desc_list[0].domain.string.value_list = NULL;
  if(argc) {
    memcpy
      (cmd->arg_desc_list + 1,
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
  sl_err = sl_flat_set_insert(app->cmd.name_set, &cmd_name, NULL);
  if(SL_NO_ERROR != sl_err) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
exit:
  return app_err;
error:
  if(cmd_name) {
    size_t j = 0;
    if(SL(hash_table_find(app->cmd.htbl, &cmd_name, ptr)), NULL != ptr) {
      SL(hash_table_erase(app->cmd.htbl, &cmd_name, &i));
      assert(0 == i);
    }
    SL(flat_set_find(app->cmd.name_set, &cmd_name, &i));
    SL(flat_set_buffer(app->cmd.name_set, &j, NULL, NULL, NULL));
    if(i != j) {
      SL(flat_set_erase(app->cmd.name_set, &cmd_name, NULL));
    }
    MEM_FREE(app->allocator, cmd_name);
  }
  if(cmd) {
    if(cmd->description)
      MEM_FREE(app->allocator, cmd->description);
    MEM_FREE(app->allocator, cmd);
  }
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
  assert(1 == nb_erased);
  SL(flat_set_erase(app->cmd.name_set, &name, NULL));
  MEM_FREE(app->allocator, cmd_name);
  if(cmd->description)
    MEM_FREE(app->allocator, cmd->description);
  MEM_FREE(app->allocator, cmd);
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_execute_command(struct app* app, const char* command)
{
  struct app_cmdarg argv[APP_MAX_CMDARGS + 1]; /* +1 <=> cmd name. */
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
    app_err = APP_COMMAND_ERROR;
    goto error;
  }
  argv[i].type = APP_CMDARG_STRING;
  argv[i].value.string = name;
  for(i = 1; i < (*cmd)->argc; ++i) {
    tkn = strtok(NULL, " \t");
    if(!tkn) {
      APP_LOG_ERR(app->logger, "%s: missing operand\n", name);
      app_err = APP_COMMAND_ERROR;
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

EXPORT_SYM enum app_error
app_command_completion
  (struct app* app,
   const char* cmd_name,
   size_t cmd_name_len,
   size_t* completion_list_len,
   const char** completion_list[])
{
  const char** name_list = NULL;
  size_t len = 0;
  enum app_error app_err = APP_NO_ERROR;

  if(!app
  || (cmd_name_len && !cmd_name)
  || !completion_list_len
  || !completion_list) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(flat_set_buffer(app->cmd.name_set, &len, NULL, NULL, (void**)&name_list));
  if(0 == cmd_name_len) {
    *completion_list_len = len;
    *completion_list = name_list;
  } else {
    #define CHARBUF_SIZE 32
    char buf[CHARBUF_SIZE];
    const char* ptr = buf;
    size_t begin = 0;
    size_t end = 0;

    if(cmd_name_len > CHARBUF_SIZE - 1) {
      app_err = APP_MEMORY_ERROR;
      goto error;
    }
    strncpy(buf, cmd_name, cmd_name_len);
    buf[cmd_name_len] = '\0';
    SL(flat_set_lower_bound(app->cmd.name_set, &ptr, &begin));
    buf[cmd_name_len] = 127;
    buf[cmd_name_len + 1] = '\0';
    SL(flat_set_upper_bound(app->cmd.name_set, &ptr, &end));
    *completion_list = name_list + begin;
    *completion_list_len = (name_list + end) - (*completion_list);
    if(0 == *completion_list_len)
      *completion_list = NULL;
    #undef CHARBUF_SIZE
  }
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
    (sizeof(const char*),
     ALIGNOF(const char*),
     sizeof(struct app_command*),
     ALIGNOF(struct app_command*),
     hash_str,
     eq_str,
     app->allocator,
     &app->cmd.htbl);
  if(SL_NO_ERROR != sl_err) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_create_flat_set
    (sizeof(const char*),
     ALIGNOF(const char*),
     cmp_str,
     app->allocator,
     &app->cmd.name_set);
  if(SL_NO_ERROR != sl_err) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  app_err = app_setup_builtin_commands(app);
  if(app_err != APP_NO_ERROR)
    goto error;

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

  if(app->cmd.name_set) {
    SL(free_flat_set(app->cmd.name_set));
    app->cmd.name_set = NULL;
  }
  if(app->cmd.htbl) {
    del_all_commands(app);
    SL(free_hash_table(app->cmd.htbl));
    app->cmd.htbl = NULL;
  }
  return APP_NO_ERROR;
}

