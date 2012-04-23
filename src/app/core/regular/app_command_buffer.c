#include "app/core/regular/app_c.h"
#include "app/core/regular/app_command_c.h"
#include "app/core/regular/app_command_buffer_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/app_command.h"
#include "app/core/app_command_buffer.h"
#include "stdlib/sl.h"
#include "stdlib/sl_string.h"
#include "sys/list.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include "sys/mem_allocator.h"
#include <stdlib.h>
#include <string.h>

#define COMMANDS_COUNT 64

struct command {
  struct list_node node;
  struct sl_string* str;
  /* Pointer toward the current command. It points whether onto this or a new
   * command which is initialized with this. Use to manage command history. */
  struct command* current;
};

struct app_command_buffer {
  struct command cmd_pool[COMMANDS_COUNT];
  struct list_node free_cmd_list;
  struct list_node used_cmd_list;
  struct ref ref;
  struct app* app;
  struct command* cmd;
  size_t cursor;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_regular_command_buffer(struct ref* ref)
{
  struct app_command_buffer* buf = NULL;
  size_t i = 0;
  assert(ref);

  buf = CONTAINER_OF(ref, struct app_command_buffer, ref);
  for(i = 0; i < COMMANDS_COUNT; ++i) {
    if(buf->cmd_pool[i].str)
      SL(free_string(buf->cmd_pool[i].str));
  }
  MEM_FREE(buf->app->allocator, buf);
}

static void
release_command_buffer(struct ref* ref)
{
  struct app* app = NULL;
  struct app_command_buffer* buf = NULL;
  assert(ref);

  buf = CONTAINER_OF(ref, struct app_command_buffer, ref);
  app = buf->app;
  release_regular_command_buffer(ref);
  APP(ref_put(app));
}

static struct command*
new_cmd(struct app_command_buffer* buf)
{
  struct list_node* node = NULL;
  struct command* cmd = NULL;
  assert(buf);
  if(is_list_empty(&buf->free_cmd_list)) {
    node = list_tail(&buf->used_cmd_list);
    list_del(node);
    cmd = CONTAINER_OF(node, struct command, node);
    if(cmd->current != cmd) {
      list_add_tail(&buf->free_cmd_list, &cmd->current->node);
      cmd->current = cmd;
    }
  } else {
    node = list_head(&buf->free_cmd_list);
    list_del(node);
    cmd = CONTAINER_OF(node, struct command, node);
    assert(cmd->current == cmd);
  }
  SL(clear_string(cmd->str));
  return cmd;
}

static void
flush_cmd(struct app_command_buffer* buf)
{
  assert(buf && buf->cmd);
  if(&buf->cmd->node != list_head(&buf->used_cmd_list)) {
    assert(buf->cmd->current != buf->cmd);
    list_move(list_head(&buf->used_cmd_list), &buf->free_cmd_list);
    list_add(&buf->used_cmd_list, &buf->cmd->current->node);
    buf->cmd->current = buf->cmd;
  }
  buf->cmd = new_cmd(buf);
  buf->cursor = 0;
  list_add(&buf->used_cmd_list, &buf->cmd->node);
}

static enum app_error
setup_cmd(struct app_command_buffer* buf)
{
  enum app_error app_err = APP_NO_ERROR;
  assert(buf && buf->cmd);

  if(&buf->cmd->node != list_head(&buf->used_cmd_list)
  && buf->cmd->current == buf->cmd) {
    enum sl_error sl_err = SL_NO_ERROR;
    const char* cstr = NULL;
    buf->cmd->current = new_cmd(buf);
    SL(string_get(buf->cmd->str, &cstr));
    sl_err = sl_string_set(buf->cmd->current->str, cstr);
    if(SL_NO_ERROR != sl_err) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
  }
exit:
  return app_err;
error:
  if(&buf->cmd->node != list_head(&buf->used_cmd_list)
  && buf->cmd->current != buf->cmd) {
    list_add_tail(&buf->free_cmd_list, &buf->cmd->current->node);
  }
  goto exit;
}

/*******************************************************************************
 *
 * Command buffer functions.
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_create_command_buffer
  (struct app* app,
   struct app_command_buffer** cmdbuf)
{
  enum app_error app_err = APP_NO_ERROR;
  app_err = app_regular_create_command_buffer(app, cmdbuf);
  if(APP_NO_ERROR != app_err)
    goto error;
  APP(ref_get(app));
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_command_buffer_ref_get(struct app_command_buffer* buf)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  ref_get(&buf->ref);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_command_buffer_ref_put(struct app_command_buffer* buf)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  ref_put(&buf->ref, release_command_buffer);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_command_buffer_write_string
  (struct app_command_buffer* buf,
   const char* str)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!buf || !str) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  assert(buf->cmd && buf->cmd->current);
  sl_err = sl_string_insert(buf->cmd->current->str, buf->cursor, str);
  if(SL_NO_ERROR != sl_err) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  buf->cursor += strlen(str);
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_command_buffer_write_char(struct app_command_buffer* buf, char ch)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!buf) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  assert(buf->cmd && buf->cmd->current);
  sl_err = sl_string_insert_char(buf->cmd->current->str, buf->cursor, ch);
  if(SL_NO_ERROR != sl_err) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  ++buf->cursor;
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_command_buffer_write_backspace(struct app_command_buffer* buf)
{
  enum app_error app_err = APP_NO_ERROR;
  if(!buf) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  if(buf->cursor > 0) {
    assert(buf->cmd && buf->cmd->current);
    --buf->cursor;
    SL(string_erase_char(buf->cmd->current->str, buf->cursor));
  }
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_command_buffer_write_suppr(struct app_command_buffer* buf)
{
  size_t len = 0;
  enum app_error app_err = APP_NO_ERROR;
  if(!buf) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  assert(buf->cmd && buf->cmd->current);
  SL(string_length(buf->cmd->current->str, &len));
  if(buf->cursor != len) {
    ++buf->cursor;
    app_err = app_command_buffer_write_backspace(buf);
  }
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_command_buffer_move_cursor(struct app_command_buffer* buf, int i)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  if(i <= 0) {
    const size_t t = (size_t)abs(i);
    buf->cursor -= MIN(t, buf->cursor);
  } else {
    size_t len = 0;
    const size_t t = (size_t)i;
    assert(buf->cmd && buf->cmd->current);
    SL(string_length(buf->cmd->current->str, &len));
    buf->cursor += MIN(t, len - buf->cursor);
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_clear_command_buffer(struct app_command_buffer* buf)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  buf->cmd = CONTAINER_OF
    (list_head(&buf->used_cmd_list), struct command, node);
  SL(clear_string(buf->cmd->str));
  buf->cursor = 0;
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_execute_command_buffer(struct app_command_buffer* buf)
{
  const char* cstr = NULL;
  size_t len = 0;
  enum app_error app_err = APP_NO_ERROR;

  if(!buf) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  assert(buf->cmd);
  SL(string_length(buf->cmd->current->str, &len));
  if(len) {
    SL(string_get(buf->cmd->current->str, &cstr));
    app_err = app_execute_command(buf->app, cstr);
    flush_cmd(buf);
    if(APP_NO_ERROR != app_err)
      goto error;
  }
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_command_buffer_history_next(struct app_command_buffer* buf)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  if(buf->cmd->node.next != &buf->used_cmd_list) {
    size_t len = 0;
    buf->cmd = CONTAINER_OF(buf->cmd->node.next, struct command, node);
    SL(string_length(buf->cmd->current->str, &len));
    setup_cmd(buf);
    buf->cursor = len;
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_command_buffer_history_prev(struct app_command_buffer* buf)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  if(buf->cmd->node.prev != &buf->used_cmd_list) {
    size_t len = 0;
    buf->cmd = CONTAINER_OF(buf->cmd->node.prev, struct command, node);
    SL(string_length(buf->cmd->current->str, &len)),
    setup_cmd(buf);
    buf->cursor = len;
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_command_buffer_clear_history(struct app_command_buffer* buf)
{
  struct list_node* head = NULL;
  struct list_node* node = NULL;
  struct list_node* tmp = NULL;
  size_t len = 0;

  if(!buf)
    return APP_INVALID_ARGUMENT;

  head = list_head(&buf->used_cmd_list);
  list_del(head);
  LIST_FOR_EACH_SAFE(node, tmp, &buf->used_cmd_list) {
    struct command* cmd = CONTAINER_OF(node, struct command, node);
    if(cmd->current != cmd) {
      list_add_tail(&buf->free_cmd_list, &cmd->current->node);
      cmd->current = cmd;
    }
    list_move_tail(node, &buf->free_cmd_list);
  }
  list_add(&buf->used_cmd_list, head);
  buf->cmd = CONTAINER_OF(head, struct command, node);
  SL(string_length(buf->cmd->str, &len));
  buf->cursor = len;
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_dump_command_buffer
  (struct app_command_buffer* buf,
   size_t* len,
   char* buffer)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  assert(buf->cmd);
  if(len) {
    SL(string_length(buf->cmd->current->str, len));
  }
  if(buffer) {
    const char* cstr = NULL;
    SL(string_get(buf->cmd->current->str, &cstr));
    strcpy(buffer, cstr);
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_command_buffer_completion
  (struct app_command_buffer* buf,
   size_t* out_len,
   const char** out_list[])
{
  #define SCRATCH_SIZE 512
  char scratch[SCRATCH_SIZE];
  const char** list = NULL;
  const char* ptr = NULL;
  const char* tkn = NULL;
  const char* cmd_name = NULL;
  size_t len = 0;
  size_t tkn_id = 0;
  size_t tkn_len = 0;
  enum app_error app_err = APP_NO_ERROR;

  if(!buf) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  if(buf->cursor > SCRATCH_SIZE - 1) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  SL(string_get(buf->cmd->current->str, &ptr));
  strncpy(scratch, ptr, buf->cursor);
  scratch[buf->cursor] = '\0';

  /* Find the token to complete. */
  cmd_name = tkn = strtok(scratch, " \t=");
  for(tkn_id = 0; NULL != (ptr = strtok(NULL, " \t=")); ++tkn_id) {
    tkn = ptr;
  }

  if(!tkn) {
    tkn_len = 0;
  } else {
    /* If the last tkn is followed by space then the arg to complete
     * is the next one. */
    tkn_len = strlen(tkn);
    if(tkn + tkn_len != scratch + buf->cursor) {
      ++tkn_id;
      tkn_len = 0;
      tkn = NULL;
    }
  }
  if(tkn_id == 0) {
    APP(command_name_completion(buf->app, cmd_name, buf->cursor, &len, &list));
  } else {
    APP(command_arg_completion(buf->app, cmd_name, tkn, tkn_len, &len, &list));
  }
  if(len != 0) {
    const size_t last = len - 1;
    size_t i = tkn_len;
    while(list[0][i] == list[last][i] && list[0][i] != '\0') {
      APP(command_buffer_write_char(buf, list[0][i]));
      ++i;
    }
    if(i != tkn_len || !last) {
      len = 0;
      list = NULL;
    }
  }
  if(out_len)
    *out_len = len;
  if(out_list)
    *out_list = list;
  #undef SCRATCH_SIZE
exit:
  return app_err;
error:
  goto exit;
}

/*******************************************************************************
 *
 * Private command buffer functions.
 *
 ******************************************************************************/
enum app_error
app_regular_create_command_buffer
  (struct app* app,
   struct app_command_buffer** cmdbuf)
{
  struct app_command_buffer* buf = NULL;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app || !cmdbuf) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  buf = MEM_CALLOC(app->allocator, 1, sizeof(struct app_command_buffer));
  if(!buf) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  ref_init(&buf->ref);
  buf->app = app;
  buf->cursor = 0;
  list_init(&buf->free_cmd_list);
  list_init(&buf->used_cmd_list);
  for(i = 0; i < COMMANDS_COUNT; ++i) {
    list_init(&buf->cmd_pool[i].node);
    sl_err = sl_create_string(NULL, app->allocator, &buf->cmd_pool[i].str);
    buf->cmd_pool[i].current = buf->cmd_pool + i;
    if(SL_NO_ERROR != sl_err) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    list_add(&buf->free_cmd_list, &buf->cmd_pool[i].node);
  }
  buf->cmd = new_cmd(buf);
  list_add(&buf->used_cmd_list, &buf->cmd->node);
exit:
  if(cmdbuf)
    *cmdbuf = buf;
  return app_err;
error:
  if(buf) {
    APP(regular_command_buffer_ref_put(buf));
    buf = NULL;
  }
  goto exit;
}

enum app_error
app_regular_command_buffer_ref_put(struct app_command_buffer* buf)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  ref_put(&buf->ref, release_regular_command_buffer);
  return APP_NO_ERROR;
}

enum app_error
app_get_command_buffer_string
  (struct app_command_buffer* buf,
   size_t* cursor,
   const char** str)
{
  if(!buf)
    return APP_INVALID_ARGUMENT;
  if(NULL != cursor)
    *cursor = buf->cursor;
  if(NULL != str)
    SL(string_get(buf->cmd->current->str, str));
  return APP_NO_ERROR;
}

