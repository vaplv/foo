#include "app/core/regular/app_builtin_commands.h"
#include "app/core/regular/app_c.h"
#include "app/core/regular/app_command_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/app_command.h"
#include "stdlib/sl_flat_set.h"
#include "stdlib/sl_hash_table.h"
#include "stdlib/sl_string.h"
#include "sys/list.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <argtable2.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct app_command {
  struct list_node node;
  size_t argc;
  void(*func)(struct app*, size_t, const struct app_cmdarg**);
  enum app_error (*completion)
    (struct app*, const char*, size_t, size_t*, const char**[]);
  struct sl_string* description;
  union app_cmdarg_domain* arg_domain;
  struct app_cmdarg** argv;
  void** arg_table;
};

#define IS_END_REACHED(desc) \
  ( (desc.type == APP_CMDARG_END.type) \
  & (desc.short_options == APP_CMDARG_END.short_options) \
  & (desc.long_options == APP_CMDARG_END.long_options) \
  & (desc.data_type == APP_CMDARG_END.data_type) \
  & (desc.glossary == APP_CMDARG_END.glossary) \
  & (desc.min_count == APP_CMDARG_END.min_count) \
  & (desc.max_count == APP_CMDARG_END.max_count) \
  & (desc.domain.integer.min == APP_CMDARG_END.domain.integer.min) \
  & (desc.domain.integer.max == APP_CMDARG_END.domain.integer.max))

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
 * Helper function.
 *
 ******************************************************************************/
static enum app_error
init_domain_and_table
  (struct app* app UNUSED,
   struct app_command* cmd,
   const struct app_cmdarg_desc argv_desc[])
{
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;
  assert(app && cmd);

  if(argv_desc) {
    void* arg = NULL;

    #define ARG(suffix, a) \
      CONCAT(arg_, suffix) \
        ((a).short_options, (a).long_options, (a).data_type, (a).glossary)
    #define LIT(suffix, a) \
      CONCAT(arg_, suffix) \
        ((a).short_options, (a).long_options, (a).glossary)
    #define ARGN(suffix, a) \
      CONCAT(CONCAT(arg_, suffix), n) \
        ((a).short_options, (a).long_options, (a).data_type, \
         (a).min_count, (a).max_count, (a).glossary)
    #define LITN(suffix, a) \
      CONCAT(CONCAT(arg_, suffix), n) \
        ((a).short_options, (a).long_options, (a).min_count, \
         (a).max_count, (a).glossary)

    for(i = 0; !IS_END_REACHED(argv_desc[i]); ++i) {
     if(argv_desc[i].min_count == 0 && argv_desc[i].max_count == 1) {
        switch(argv_desc[i].type) {
          case APP_CMDARG_INT: arg = ARG(int0, argv_desc[i]); break;
          case APP_CMDARG_FILE: arg = ARG(file0, argv_desc[i]); break;
          case APP_CMDARG_FLOAT: arg = ARG(dbl0, argv_desc[i]); break;
          case APP_CMDARG_STRING: arg = ARG(str0, argv_desc[i]); break;
          case APP_CMDARG_LITERAL: arg = LIT(lit0, argv_desc[i]); break;
          default: assert(0); break;
        }
      } else if(argv_desc[i].max_count == 1) {
        switch(argv_desc[i].type) {
          case APP_CMDARG_INT: arg = ARG(int1, argv_desc[i]); break;
          case APP_CMDARG_FILE: arg = ARG(file1, argv_desc[i]); break;
          case APP_CMDARG_FLOAT: arg = ARG(dbl1, argv_desc[i]); break;
          case APP_CMDARG_STRING: arg = ARG(str1, argv_desc[i]); break;
          case APP_CMDARG_LITERAL: arg = LIT(lit1, argv_desc[i]); break;
          default: assert(0); break;
        }
      } else {
        switch(argv_desc[i].type) {
          case APP_CMDARG_INT: arg = ARGN(int, argv_desc[i]); break;
          case APP_CMDARG_FILE: arg = ARGN(file, argv_desc[i]); break;
          case APP_CMDARG_FLOAT: arg = ARGN(dbl, argv_desc[i]); break;
          case APP_CMDARG_STRING: arg = ARGN(str, argv_desc[i]); break;
          case APP_CMDARG_LITERAL: arg = LITN(lit, argv_desc[i]); break;
          default: assert(0); break;
        }
      }
      cmd->arg_table[i] = arg;
      cmd->arg_domain[i + 1] = argv_desc[i].domain; /* +1 <=> command name. */
    }
  }

  cmd->arg_table[i] = arg_end(16);
  if(arg_nullcheck(cmd->arg_table)) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }

  #undef ARG
  #undef LIT
  #undef ARGN
  #undef LITN

exit:
  return app_err;
error:
  goto exit;
}

static FINLINE void
set_optvalue_flag(struct app_command* cmd, const bool val)
{
  size_t argv_id = 0;
  size_t tbl_id = 0;
  assert(cmd);

  #define SET_OPTVAL(arg, val) \
    do { \
      if(val) \
        (arg)->hdr.flag |= ARG_HASOPTVALUE; \
      else \
        (arg)->hdr.flag &= ~ARG_HASOPTVALUE; \
    } while(0)

  for(argv_id = 1, tbl_id = 0; argv_id < cmd->argc; ++argv_id, ++tbl_id) {
    switch(cmd->argv[argv_id]->type) {
      case APP_CMDARG_INT:
        SET_OPTVAL((struct arg_int*)cmd->arg_table[tbl_id], val);
        break;
      case APP_CMDARG_FILE:
        SET_OPTVAL((struct arg_file*)cmd->arg_table[tbl_id], val);
        break;
      case APP_CMDARG_FLOAT:
        SET_OPTVAL((struct arg_dbl*)cmd->arg_table[tbl_id], val);
        break;
      case APP_CMDARG_STRING:
        SET_OPTVAL((struct arg_str*)cmd->arg_table[tbl_id], val);
        break;
      case APP_CMDARG_LITERAL:
        SET_OPTVAL((struct arg_lit*)cmd->arg_table[tbl_id], val);
        break;
      default: assert(0); break;
    }
  }
  #undef SET_OPTVAL
}

static int
defined_args_count(struct app_command* cmd)
{
  int count = 0;
  size_t argv_id = 0;
  size_t tbl_id = 0;
  assert(cmd);

  for(argv_id = 1, tbl_id = 0; argv_id < cmd->argc; ++argv_id, ++tbl_id) {
    switch(cmd->argv[argv_id]->type) {
      case APP_CMDARG_INT:
        count += ((struct arg_int*)cmd->arg_table[tbl_id])->count;
        break;
      case APP_CMDARG_FILE:
        count += ((struct arg_file*)cmd->arg_table[tbl_id])->count;
        break;
      case APP_CMDARG_FLOAT:
        count += ((struct arg_dbl*)cmd->arg_table[tbl_id])->count;
        break;
      case APP_CMDARG_STRING:
        count += ((struct arg_str*)cmd->arg_table[tbl_id])->count;
        break;
      case APP_CMDARG_LITERAL:
        count += ((struct arg_lit*)cmd->arg_table[tbl_id])->count;
        break;
      default: assert(0); break;
    }
  }
  return count;
}

static enum app_error
setup_cmd_arg(struct app* app, struct app_command* cmd, const char* name)
{
  size_t arg_id = 0;
  enum app_error app_err = APP_NO_ERROR;

  assert(cmd && cmd->argc && cmd->argv[0]->type == APP_CMDARG_STRING);
  cmd->argv[0]->value_list[0].is_defined = true;
  cmd->argv[0]->value_list[0].data.string = name;
  for(arg_id = 1; arg_id < cmd->argc; ++arg_id) {
    const char** value_list = NULL;
    const char* str = NULL;
    const size_t arg_tbl_id = arg_id - 1; /* -1 <=> arg name. */
    int val_id = 0;
    bool isdef = true;

    for(val_id=0; isdef && (size_t)val_id<cmd->argv[arg_id]->count; ++val_id) {
      switch(cmd->argv[arg_id]->type) {
        case APP_CMDARG_STRING:
          isdef = val_id < ((struct arg_str*)cmd->arg_table[arg_tbl_id])->count;
          if(isdef) {
            str = ((struct arg_str*)(cmd->arg_table[arg_tbl_id]))->sval[val_id];
            /* Check the string domain. */
            value_list = cmd->arg_domain[arg_id].string.value_list;
            if(value_list == NULL) {
              cmd->argv[arg_id]->value_list[val_id].data.string = str;
            } else {
              size_t i = 0;
              for(i = 0; value_list[i] != NULL; ++i) {
                if(strcmp(str, value_list[i]) == 0)
                  break;
              }
              if(value_list[i] != NULL) {
                cmd->argv[arg_id]->value_list[val_id].data.string = str;
              } else {
                APP_PRINT_ERR
                  (app->logger, "%s: unexpected option value `%s'\n",name, str);
                app_err = APP_COMMAND_ERROR;
                goto error;
              }
            }
          }
          break;
        case APP_CMDARG_FILE:
          isdef = val_id< ((struct arg_file*)cmd->arg_table[arg_tbl_id])->count;
          if(isdef) {
            cmd->argv[arg_id]->value_list[val_id].data.string =
             ((struct arg_file*)(cmd->arg_table[arg_tbl_id]))->filename[val_id];
          }
          break;
        case APP_CMDARG_INT:
          isdef = val_id < ((struct arg_int*)cmd->arg_table[arg_tbl_id])->count;
          if(isdef) {
            cmd->argv[arg_id]->value_list[val_id].data.integer = MAX(MIN(
              ((struct arg_int*)(cmd->arg_table[arg_tbl_id]))->ival[val_id],
              cmd->arg_domain[arg_id].integer.max),
              cmd->arg_domain[arg_id].integer.min);
          }
          break;
        case APP_CMDARG_FLOAT:
          isdef = val_id < ((struct arg_dbl*)cmd->arg_table[arg_tbl_id])->count;
          if(isdef) {
            cmd->argv[arg_id]->value_list[val_id].data.real = MAX(MIN(
              ((struct arg_dbl*)(cmd->arg_table[arg_tbl_id]))->dval[val_id],
              cmd->arg_domain[arg_id].real.max),
              cmd->arg_domain[arg_id].real.min);
          }
          break;
        case APP_CMDARG_LITERAL:
          isdef = val_id < ((struct arg_lit*)cmd->arg_table[arg_tbl_id])->count;
          break;
        default:
          assert(0);
          break;
      }
      cmd->argv[arg_id]->value_list[val_id].is_defined = isdef;
    }
  }
exit:
  return app_err;
error:
  goto exit;
}

static enum app_error
register_command
  (struct app* app,
   struct app_command* cmd,
   const char* name)
{
  struct list_node* list = NULL;
  char* cmd_name = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool is_inserted_in_htbl = false;
  bool is_inserted_in_fset = false;
  assert(app && cmd && name);

  SL(hash_table_find(app->cmd.htbl, &name, (void**)&list));

  /* Register the command agains the command system if it does not exist. */
  if(list == NULL) {
    cmd_name = MEM_CALLOC(app->allocator, strlen(name) + 1, sizeof(char));
    if(NULL == cmd_name) {
      app_err = APP_MEMORY_ERROR;
      goto error;
    }
    strcpy(cmd_name, name);

    sl_err = sl_hash_table_insert
      (app->cmd.htbl, &cmd_name, (struct list_node[]){{NULL, NULL}});
    if(SL_NO_ERROR != sl_err) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    is_inserted_in_htbl = true;

    SL(hash_table_find(app->cmd.htbl, &cmd_name, (void**)&list));
    assert(list != NULL);
    list_init(list);

    sl_err = sl_flat_set_insert(app->cmd.name_set, &cmd_name, NULL);
    if(SL_NO_ERROR != sl_err) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    is_inserted_in_fset = true;
  }

  list_add(list, &cmd->node);

exit:
  return app_err;
error:
  if(cmd_name) {
    size_t i = 0;

    if(is_inserted_in_htbl) {
      SL(hash_table_erase(app->cmd.htbl, &cmd_name, &i));
      assert(1 == i);
    }
    if(is_inserted_in_fset) {
      SL(flat_set_erase(app->cmd.name_set, &cmd_name, NULL));
    }
    MEM_FREE(app->allocator, cmd_name);
  }
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
    struct list_node* list = (struct list_node*)it.pair.data;
    struct list_node* pos = NULL;
    struct list_node* tmp = NULL;
    size_t i = 0;

    assert(is_list_empty(list) == false);

    LIST_FOR_EACH_SAFE(pos, tmp, list) {
      struct app_command* cmd = CONTAINER_OF(pos, struct app_command, node);
      list_del(pos);

      if(cmd->description)
        SL(free_string(cmd->description));
      for(i = 0; i < cmd->argc; ++i) {
        MEM_FREE(app->allocator, cmd->argv[i]);
      }
      MEM_FREE(app->allocator, cmd->argv);
      MEM_FREE(app->allocator, cmd->arg_domain);
      arg_freetable(cmd->arg_table, cmd->argc + 1); /* +1 <=> arg_end. */
      MEM_FREE(app->allocator, cmd->arg_table);
      MEM_FREE(app->allocator, cmd);
    }

    MEM_FREE(app->allocator, (*(char**)it.pair.key));
    SL(hash_table_it_next(&it, &b));
  }
  SL(hash_table_clear(app->cmd.htbl));
}

/*******************************************************************************
 *
 * Command functions
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_add_command
  (struct app* app,
   const char* name,
   void (*func)(struct app*, size_t argc, const struct app_cmdarg** argv),
   enum app_error (*completion_func) /* May be NULL.*/
    (struct app*, const char*, size_t, size_t*, const char**[]),
   const struct app_cmdarg_desc argv_desc[],
   const char* description)
{
  struct app_command* cmd = NULL;
  size_t argc = 0;
  size_t buffer_len = 0;
  size_t arg_id = 0;
  size_t desc_id = 0;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app || !name || !func) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  cmd = MEM_CALLOC(app->allocator, 1, sizeof(struct app_command));
  if(NULL == cmd) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  list_init(&cmd->node);
  cmd->func = func;

  if(argv_desc != NULL) {
    for(argc = 0; !IS_END_REACHED(argv_desc[argc]); ++argc) {
      if(argv_desc[argc].min_count > argv_desc[argc].max_count
      || argv_desc[argc].max_count == 0
      || argv_desc[argc].type == APP_NB_CMDARG_TYPES) {
        app_err = APP_INVALID_ARGUMENT;
        goto error;
      }
      buffer_len += argv_desc[argc].max_count;
    }
  }
  ++argc; /* +1 <=> command name. */

  /* Create the command arg table, arg domain,  and argv container. */
  cmd->arg_table = MEM_CALLOC
    (app->allocator, argc + 1 /* +1 <=> arg_end */, sizeof(void*));
  if(NULL == cmd->arg_table) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  cmd->arg_domain = MEM_CALLOC
    (app->allocator, argc, sizeof(union app_cmdarg_domain));
  if(NULL == cmd->arg_domain) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  cmd->argv = MEM_CALLOC(app->allocator, argc, sizeof(struct app_cmdarg*));
  if(NULL == cmd->argv) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }

  /* Setup the arg domain and table. */
  app_err = init_domain_and_table(app, cmd, argv_desc);
  if(app_err != APP_NO_ERROR)
    goto error;

  /* Setup the command name arg. */
  cmd->argv[0] = MEM_CALLOC
    (app->allocator, 1,
     sizeof(struct app_cmdarg) + sizeof(struct app_cmdarg_value));
  if(NULL == cmd->argv[0]) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  cmd->argv[0]->type = APP_CMDARG_STRING;
  cmd->argv[0]->count = 1;

  /* Setup the remaining args. */
  for(arg_id = 1, desc_id = 0; arg_id < argc; ++arg_id, ++desc_id) {
    cmd->argv[arg_id] = MEM_CALLOC
      (app->allocator, 1,
       sizeof(struct app_cmdarg)
       + argv_desc[desc_id].max_count * sizeof(struct app_cmdarg_value));
    if(NULL == cmd->argv[arg_id]) {
      app_err = APP_MEMORY_ERROR;
      goto error;
    }
    cmd->argv[arg_id]->type = argv_desc[desc_id].type;
    cmd->argv[arg_id]->count = argv_desc[desc_id].max_count;
  }
  cmd->argc = argc;

  /* Setup the command description. */
  if(NULL == description) {
    cmd->description = NULL;
  } else {
    sl_err = sl_create_string(description, app->allocator, &cmd->description);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
  }
  cmd->completion = completion_func;

  /* Register the command against the command system. */
  app_err = register_command(app, cmd, name);
  if(app_err != APP_NO_ERROR)
    goto error;

exit:
  return app_err;
error:
  /* The command registration is the last action, i.e. the command is
   * registered only if no error occurs. It is thus useless to handle command
   * registration in the error management. */
  if(cmd) {
    if(cmd->description)
      SL(free_string(cmd->description));
    if(cmd->arg_table) {
      for(i = 0; i < argc + 1 /* +1 <=> arg_end */ ; ++i) {
        if(cmd->arg_table[i])
          free(cmd->arg_table[i]);
      }
      MEM_FREE(app->allocator, cmd->arg_table);
    }
    if(cmd->argv) {
      for(i = 0; i < argc; ++i) {
        if(cmd->argv[i])
          free(cmd->argv[i]);
      }
      MEM_FREE(app->allocator, cmd->argv);
    }
    MEM_FREE(app->allocator, cmd);
    cmd = NULL;
  }
  goto exit;
}

EXPORT_SYM enum app_error
app_del_command(struct app* app, const char* name)
{
  struct sl_pair pair;
  struct list_node* list = NULL;
  struct list_node* pos = NULL;
  struct list_node* tmp = NULL;
  char* cmd_name = NULL;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;

  if(!app || !name) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  /* Unregister the command. */
  SL(hash_table_find_pair(app->cmd.htbl, &name, &pair));
  if(!SL_IS_PAIR_VALID(&pair)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  /* Free the command syntaxes. */
  list = (struct list_node*)pair.data;
  LIST_FOR_EACH_SAFE(pos, tmp, list) {
    struct app_command* cmd = CONTAINER_OF(pos, struct app_command, node);
    list_del(pos);

    if(cmd->description)
      SL(free_string(cmd->description));
    for(i = 0; i < cmd->argc; ++i) {
      MEM_FREE(app->allocator, cmd->argv[i]);
    }
    MEM_FREE(app->allocator, cmd->argv);
    MEM_FREE(app->allocator, cmd->arg_domain);
    arg_freetable(cmd->arg_table, cmd->argc + 1); /* +1 <=> arg_end. */
    MEM_FREE(app->allocator, cmd->arg_table);
    MEM_FREE(app->allocator, cmd);
  }


  /* Free the command name. */
  cmd_name = *(char**)pair.key;
  SL(flat_set_erase(app->cmd.name_set, &cmd_name, NULL));
  SL(hash_table_erase(app->cmd.htbl, &cmd_name, &i));
  assert(1 == i);
  MEM_FREE(app->allocator, cmd_name);

exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_has_command(struct app* app, const char* name, bool* has_command)
{
  void* ptr = NULL;
  if(!app || !name || !has_command)
    return APP_INVALID_ARGUMENT;
  SL(hash_table_find(app->cmd.htbl, &name, &ptr));
  *has_command = (ptr != NULL);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_execute_command(struct app* app, const char* command)
{
  #define MAX_ARG_COUNT 128
  char* argv[MAX_ARG_COUNT];
  struct list_node* command_list = NULL;
  struct list_node* node = NULL;
  struct app_command* valid_cmd = NULL;
  char* name = NULL;
  char* ptr = NULL;
  size_t argc = 0;
  enum app_error app_err = APP_NO_ERROR;
  int min_nerror = 0;

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
  SL(hash_table_find(app->cmd.htbl, &name, (void**)&command_list));
  if(!command_list) {
    APP_PRINT_ERR(app->logger, "%s: command not found\n", name);
    app_err = APP_COMMAND_ERROR;
    goto error;
  }
  argv[0] = name;
  for(argc = 1; NULL != (ptr = strtok(NULL, " \t")); ++argc) {
    if(argc >= MAX_ARG_COUNT) {
      app_err = APP_MEMORY_ERROR;
      goto error;
    }
    argv[argc] = ptr;
  }

  min_nerror = INT_MAX;
  LIST_FOR_EACH(node, command_list) {
    struct app_command* cmd = CONTAINER_OF(node, struct app_command, node);
    int nerror = 0;

    assert(cmd->argc > 0);
    nerror = arg_parse(argc, argv, cmd->arg_table);

    if(nerror <= min_nerror) {
      if(nerror < min_nerror) {
        min_nerror = nerror;
        rewind(app->cmd.stream);
      }
      fprintf(app->cmd.stream, "\n%s", name);
      arg_print_syntaxv(app->cmd.stream, cmd->arg_table, "\n");
      arg_print_errors
        (app->cmd.stream, (struct arg_end*)cmd->arg_table[cmd->argc - 1], name);
    }
    if(nerror == 0) {
      valid_cmd = cmd;
      break;
    }
  }

  if(min_nerror != 0) {
    long fpos = 0;
    size_t size =0;
    size_t nb = 0;

    fpos = ftell(app->cmd.stream);
    size = MIN((size_t)fpos, sizeof(app->cmd.scratch)/sizeof(char) - 1);
    rewind(app->cmd.stream);
    nb = fread(app->cmd.scratch, size, 1, app->cmd.stream);
    assert(nb == 1);
    app->cmd.scratch[size] = '\0';
    APP_PRINT_ERR(app->logger, "%s", app->cmd.scratch);
    app_err = APP_COMMAND_ERROR;
    goto error;
  }

  /* Setup the args and invoke the commands. */
  app_err = setup_cmd_arg(app, valid_cmd, name);
  if(app_err != APP_NO_ERROR)
    goto error;
  valid_cmd->func
    (app, valid_cmd->argc, (const struct app_cmdarg**)valid_cmd->argv);

  #undef MAX_ARG_COUNT
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_man_command
  (struct app* app,
   const char* name,
   size_t* len,
   size_t max_buf_len,
   char* buffer)
{
  struct list_node* cmd_list = NULL;
  struct list_node* node = NULL;
  enum app_error app_err = APP_NO_ERROR;
  long fpos = 0;

  if(!app || !name || (max_buf_len && !buffer)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  SL(hash_table_find(app->cmd.htbl, &name, (void**)&cmd_list));
  if(!cmd_list) {
    APP_PRINT_ERR(app->logger, "%s: command not found\n", name);
    app_err = APP_COMMAND_ERROR;
    goto error;
  }
  assert(is_list_empty(cmd_list) == false);

  rewind(app->cmd.stream);
  LIST_FOR_EACH(node, cmd_list) {
    struct app_command* cmd = CONTAINER_OF(node, struct app_command, node);

    if(node != list_head(cmd_list))
       fprintf(app->cmd.stream, "\n");

    fprintf(app->cmd.stream, "%s", name);
    arg_print_syntaxv(app->cmd.stream, cmd->arg_table, "\n");
    if(cmd->description) {
      const char* cstr = NULL;
      SL(string_get(cmd->description, &cstr));
      fprintf(app->cmd.stream, "%s\n", cstr);
    }
    arg_print_glossary(app->cmd.stream, cmd->arg_table, NULL);
    fpos += ftell(app->cmd.stream);
    assert(fpos > 0);
  }

  if(len)
    *len = fpos / sizeof(char);
  if(buffer && max_buf_len) {
    const size_t size = MIN((max_buf_len - 1) * sizeof(char), (size_t)fpos);
    size_t nb = 0;

    fflush(app->cmd.stream);
    rewind(app->cmd.stream);
    nb = fread(buffer, size, 1, app->cmd.stream);
    assert(nb == 1);
    buffer[size/sizeof(char)] = '\0';
  }
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_command_arg_completion
  (struct app* app,
   const char* cmd_name,
   const char* arg_str,
   size_t arg_str_len,
   size_t hint_argc,
   char* hint_argv[],
   size_t* completion_list_len,
   const char** completion_list[])
{
  struct list_node* cmd_list = NULL;
  enum app_error app_err = APP_NO_ERROR;

  if(!app
  || !cmd_name
  || (arg_str_len && !arg_str)
  || (hint_argc && !hint_argv)
  || !completion_list_len
  || !completion_list) {
    app_err =  APP_INVALID_ARGUMENT;
    goto error;
  }
  /* Default completion list value. */
  *completion_list_len = 0;
  *completion_list = NULL;

  SL(hash_table_find(app->cmd.htbl, &cmd_name, (void**)&cmd_list));
  if(cmd_list != NULL) {
    struct app_command* cmd = NULL;
    assert(is_list_empty(cmd_list) == false);

    /* No multi syntax. */
    if(list_head(cmd_list) == list_tail(cmd_list)) { 
      cmd = CONTAINER_OF(list_head(cmd_list), struct app_command, node);
      if(cmd->completion) {
        app_err = cmd->completion
          (app, arg_str, arg_str_len, completion_list_len, completion_list);
        if(app_err != APP_NO_ERROR)
          goto error;
      }
    /* Multi syntax. */
    } else { 
      struct app_command* valid_cmd = NULL;
      struct list_node* node = NULL;
      size_t nb_valid_cmd = 0;
      int min_nerror = INT_MAX;
      int max_ndefargs = INT_MIN;

      LIST_FOR_EACH(node, cmd_list) {
        int nerror = 0;
        int ndefargs = 0;

        cmd = CONTAINER_OF(node, struct app_command, node);
        set_optvalue_flag(cmd, true);
        nerror = arg_parse(hint_argc, hint_argv, cmd->arg_table);
        ndefargs = defined_args_count(cmd);

        /* Define as the completion function the one defined by the command
         * syntax which match the best the hint arguments. If the minimal
         * number of parsing error is obtained by several syntaxes, we select
         * the syntax which have the maximum of its argument defined by the
         * hint command. */
        if(nerror < min_nerror
        || (nerror == min_nerror && ndefargs > max_ndefargs)) {
          valid_cmd = cmd;
          nb_valid_cmd = 0;
        }
        min_nerror = MIN(nerror, min_nerror);
        max_ndefargs = MAX(ndefargs, max_ndefargs);
        nb_valid_cmd += ((ndefargs == max_ndefargs) & (nerror == min_nerror));
        set_optvalue_flag(cmd, false);
      }
      /* Perform the completion only if an unique syntax match the previous
       * completion heuristic and and if its completion process is defined. */
      if(nb_valid_cmd == 1 && valid_cmd->completion) {
        app_err = valid_cmd->completion
          (app, arg_str, arg_str_len, completion_list_len, completion_list);
      }
    }
  }
  if(*completion_list_len == 0)
    *completion_list = NULL;
exit:
  return app_err;
error:
  if(completion_list_len)
    *completion_list_len = 0;
  if(completion_list)
    *completion_list = NULL;
  goto exit;
}

EXPORT_SYM enum app_error
app_command_name_completion
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
     sizeof(struct list_node),
     ALIGNOF(struct list_node),
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
  app->cmd.stream = tmpfile();
  if(!app->cmd.stream) {
    app_err = APP_IO_ERROR;
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

  if(app->cmd.htbl) {
    del_all_commands(app);
    SL(free_hash_table(app->cmd.htbl));
    app->cmd.htbl = NULL;
  }
  if(app->cmd.name_set) {
    SL(free_flat_set(app->cmd.name_set));
    app->cmd.name_set = NULL;
  }
  if(app->cmd.stream)
    fclose(app->cmd.stream);
  return APP_NO_ERROR;
}
