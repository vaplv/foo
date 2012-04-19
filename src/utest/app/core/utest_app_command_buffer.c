#include "app/core/app.h"
#include "app/core/app_command.h"
#include "app/core/app_command_buffer.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define BAD_ARG APP_INVALID_ARGUMENT
#define CMD_ERR APP_COMMAND_ERROR
#define OK APP_NO_ERROR
#define BUFFER_SIZE 512

#define CHECK_CMDBUF(buf, len, dump, str) \
  do { \
    CHECK(app_dump_command_buffer(buf, &len, dump), OK); \
    CHECK(strcmp(dump, str), 0); \
    CHECK(strlen(dump), len); \
  } while(0)

static const char* options[] = { "foo", "opt", "opt0", "optionA", "optionB" };

static struct app_cmdarg_value_list
cmd_option_list
  (struct app* app UNUSED,
   const char* input,
   size_t input_len)
{
  struct app_cmdarg_value_list option_list;
  const size_t len = sizeof(options)/sizeof(const char*);
  memset(&option_list, 0, sizeof(option_list));

  if(!input || !input_len) {
    option_list.buffer = options;
    option_list.length = len;
  } else {
    size_t i = 0;
    size_t j = 0;
    for(i = 0; i < len && strncmp(options[i], input, input_len) < 0; ++i);
    for(j = i; j < len && strncmp(options[j], input, input_len) == 0; ++j);
    option_list.buffer = options + i;
    option_list.length = j - i;
  }
  return option_list;
}

static void
foo(struct app* app UNUSED, size_t argc, struct app_cmdarg* argv UNUSED)
{
  size_t i = 0;
  bool b = false;
  CHECK(argc, 2);
  CHECK(argv[0].type, APP_CMDARG_STRING);
  CHECK(argv[1].type, APP_CMDARG_STRING);
  CHECK(strcmp(argv[0].value.string, "foo__"), 0);
  for(i = 0, b = false; !b && i < sizeof(options)/sizeof(const char*); ++i) {
    b = !strcmp(options[i], argv[1].value.string);
  }
  CHECK(b, true);
}

int
main(int argc, char** argv)
{
  const char** list = NULL; 
  char dump[BUFFER_SIZE] = { [0] = 'a', [1] = '\0' };
  struct app_args args = { NULL, NULL, NULL, NULL };
  struct app* app = NULL;
  struct app_command_buffer* buf = NULL;
  size_t len = SIZE_MAX;
  size_t len2 = SIZE_MAX;

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    return -1;
  }
  args.render_driver = argv[1];

  CHECK(app_init(&args, &app), OK);

  CHECK(app_create_command_buffer(NULL, NULL), BAD_ARG);
  CHECK(app_create_command_buffer(app, NULL), BAD_ARG);
  CHECK(app_create_command_buffer(NULL, &buf), BAD_ARG);
  CHECK(app_create_command_buffer(app, &buf), OK);

  CHECK(app_dump_command_buffer(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_dump_command_buffer(buf, NULL, NULL), OK);
  CHECK(app_dump_command_buffer(NULL, &len, NULL), BAD_ARG);
  CHECK(app_dump_command_buffer(buf, &len, NULL), OK);
  CHECK(len, 0);
  len = SIZE_MAX;
  CHECK(app_dump_command_buffer(NULL, NULL, dump), BAD_ARG);
  CHECK(app_dump_command_buffer(buf, NULL, dump), OK);
  CHECK(strlen(dump), 0);
  dump[0] = 'a', dump[1] = '\0';
  CHECK(app_dump_command_buffer(NULL, &len, dump), BAD_ARG);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 0);
  CHECK(strlen(dump), 0);

  CHECK(app_command_buffer_write_string(NULL, NULL), BAD_ARG);
  CHECK(app_command_buffer_write_string(buf, NULL), BAD_ARG);
  CHECK(app_command_buffer_write_string(NULL, "foo"), BAD_ARG);
  CHECK(app_command_buffer_write_string(buf, "foo"), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK_CMDBUF(buf, len, dump, "foo");
  CHECK(len, 3);

  CHECK(app_command_buffer_write_string(buf, ". Hello world"), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK_CMDBUF(buf, len, dump, "foo. Hello world");
  CHECK(len, 16);

  CHECK(app_command_buffer_write_char(NULL, '!'), BAD_ARG);
  CHECK(app_command_buffer_write_char(buf, '!'), OK);
  CHECK_CMDBUF(buf, len, dump, "foo. Hello world!");
  CHECK(len, 17);

  CHECK(app_command_buffer_write_char(buf, '.'), OK);
  CHECK_CMDBUF(buf, len, dump, "foo. Hello world!.");
  CHECK(len, 18);

  CHECK(app_command_buffer_write_backspace(NULL), BAD_ARG);
  CHECK(app_command_buffer_write_backspace(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "foo. Hello world!");
  CHECK(len, 17);

  CHECK(app_command_buffer_write_suppr(NULL), BAD_ARG);
  CHECK(app_command_buffer_write_suppr(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "foo. Hello world!");
  CHECK(len, 17);

  CHECK(app_command_buffer_move_cursor(NULL, 1), BAD_ARG);
  CHECK(app_command_buffer_move_cursor(buf, 1), OK);
  CHECK(app_command_buffer_write_string(buf, " :)"), OK);
  CHECK_CMDBUF(buf, len, dump, "foo. Hello world! :)");
  CHECK(len, 20);

  CHECK(app_command_buffer_move_cursor(buf, INT_MAX), OK);
  CHECK(app_command_buffer_write_string(buf, "..."), OK);
  CHECK_CMDBUF(buf, len, dump, "foo. Hello world! :)...");
  CHECK(len, 23);

  CHECK(app_command_buffer_move_cursor(buf, -4), OK);
  CHECK(app_command_buffer_write_char(buf, '-'), OK);
  CHECK_CMDBUF(buf, len, dump, "foo. Hello world! :-)...");
  CHECK(len, 24);

  CHECK(app_command_buffer_write_suppr(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "foo. Hello world! :-...");
  CHECK(len, 23);

  CHECK(app_command_buffer_write_string(buf, "/!"), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK_CMDBUF(buf, len, dump, "foo. Hello world! :-/!...");
  CHECK(len, 25);

  CHECK(app_command_buffer_move_cursor(buf, -18), OK);
  CHECK(app_command_buffer_write_backspace(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "foo Hello world! :-/!...");
  CHECK(len, 24);
  CHECK(strcmp(dump, "foo Hello world! :-/!..."), 0);

  CHECK(app_command_buffer_write_string(buf, "!!!!"), OK);
  CHECK_CMDBUF(buf, len, dump, "foo!!!! Hello world! :-/!...");
  CHECK(len, 28);

  CHECK(app_command_buffer_move_cursor(buf, INT_MIN), OK);
  CHECK(app_command_buffer_write_backspace(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "foo!!!! Hello world! :-/!...");
  CHECK(len, 28);

  CHECK(app_command_buffer_write_suppr(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "oo!!!! Hello world! :-/!...");
  CHECK(len, 27);

  CHECK(app_command_buffer_write_char(buf, 'F'), OK);
  CHECK_CMDBUF(buf, len, dump, "Foo!!!! Hello world! :-/!...");
  CHECK(len, 28);

  CHECK(app_clear_command_buffer(NULL), BAD_ARG);
  CHECK(app_clear_command_buffer(buf), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 0);
  CHECK(dump[0], '\0');

  CHECK(app_command_buffer_write_string(buf, "ls"), OK);
  CHECK_CMDBUF(buf, len, dump, "ls");
  CHECK(len, 2);

  CHECK(app_command_buffer_move_cursor(buf, INT_MIN), OK);
  CHECK(app_command_buffer_write_string(buf, "help "), OK);
  CHECK_CMDBUF(buf, len, dump, "help ls");
  CHECK(len, 7);

  CHECK(app_execute_command_buffer(NULL), BAD_ARG);
  CHECK(app_execute_command_buffer(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "");
  CHECK(len, 0);
  CHECK(dump[0], '\0');
  CHECK(app_execute_command_buffer(buf), OK);

  CHECK(app_command_buffer_write_string(buf, "wrong_command"), OK);
  CHECK_CMDBUF(buf, len, dump, "wrong_command");
  CHECK(app_execute_command_buffer(buf), CMD_ERR);

  CHECK(app_command_buffer_clear_history(NULL), BAD_ARG);
  CHECK(app_command_buffer_clear_history(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "");

  CHECK(app_command_buffer_history_next(NULL), BAD_ARG);
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "");

  CHECK(app_command_buffer_write_string(buf, "cmd0"), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0");
  CHECK(app_command_buffer_move_cursor(buf, -1), OK);
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0");
  CHECK(app_command_buffer_history_prev(NULL), BAD_ARG);
  CHECK(app_command_buffer_history_prev(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0");
  CHECK(app_execute_command_buffer(buf), CMD_ERR);
  CHECK_CMDBUF(buf, len, dump, "");

  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0");
  CHECK(app_command_buffer_history_prev(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "");
  CHECK(app_command_buffer_history_prev(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "");
  CHECK(app_command_buffer_write_string(buf, "cmd1"), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0");
  CHECK(app_command_buffer_write_backspace(buf), OK);
  CHECK(app_command_buffer_write_char(buf, '1'), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1");
  CHECK(app_command_buffer_history_prev(buf), OK);
  CHECK(app_command_buffer_write_string(buf, "aaa"), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1aaa");
  CHECK(app_command_buffer_move_cursor(buf, -6), OK);
  CHECK(app_command_buffer_write_char(buf, '_'), OK);
  CHECK_CMDBUF(buf, len, dump, "c_md1aaa");
  CHECK(app_command_buffer_history_prev(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "c_md1aaa");
  CHECK(app_command_buffer_write_char(buf, '*'), OK);
  CHECK_CMDBUF(buf, len, dump, "c_*md1aaa");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1");
  CHECK(app_command_buffer_write_char(buf, 'a'), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1a");
  CHECK(app_command_buffer_write_backspace(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1");
  CHECK(app_execute_command_buffer(buf), CMD_ERR);
  CHECK_CMDBUF(buf, len, dump, "");

  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0");
  CHECK(app_command_buffer_history_prev(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1");
  CHECK(app_command_buffer_history_prev(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "");
  CHECK(app_command_buffer_history_prev(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "");
  CHECK(app_command_buffer_write_string(buf, "Foo"), OK);
  CHECK_CMDBUF(buf, len, dump, "Foo");
  CHECK(app_command_buffer_history_prev(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "Foo");
  CHECK(app_command_buffer_move_cursor(buf, INT_MIN), OK);
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK(app_command_buffer_write_string(buf, "abcd"), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1abcd");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK(app_command_buffer_write_string(buf, " 0123"), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0 0123");
  CHECK(app_clear_command_buffer(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1abcd");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0 0123");
  CHECK(app_command_buffer_write_string(buf, "md2"), OK);
  CHECK(app_command_buffer_move_cursor(buf, -3), OK);
  CHECK(app_command_buffer_write_char(buf, 'c'), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0 0123cmd2");
  CHECK(app_command_buffer_history_prev(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1abcd");
  CHECK(app_command_buffer_history_prev(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "");
  CHECK(app_command_buffer_write_string(buf, "cmd2"), OK);
  CHECK(app_execute_command_buffer(buf), CMD_ERR);

  CHECK_CMDBUF(buf, len, dump, "");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd2");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1abcd");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0 0123cmd2");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0 0123cmd2");
  CHECK(app_command_buffer_history_prev(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1abcd");
  CHECK(app_command_buffer_move_cursor(buf, -5), OK);
  CHECK(app_command_buffer_write_suppr(buf), OK);
  CHECK(app_command_buffer_write_suppr(buf), OK);
  CHECK(app_command_buffer_write_suppr(buf), OK);
  CHECK(app_command_buffer_write_suppr(buf), OK);
  CHECK(app_command_buffer_write_suppr(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd");
  CHECK(app_command_buffer_write_string(buf, "3"), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd3");
  CHECK(app_execute_command_buffer(buf), CMD_ERR);

  CHECK_CMDBUF(buf, len, dump, "");
  CHECK(app_command_buffer_write_string(buf, "cmd4"), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd4");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd3");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd2");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd1");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd0 0123cmd2");
  CHECK(app_command_buffer_clear_history(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd4");
  CHECK(app_command_buffer_write_char(buf, 'a'), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd4a");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd4a");
  CHECK(app_command_buffer_history_next(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd4a");
  CHECK(app_command_buffer_history_prev(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd4a");
  CHECK(app_command_buffer_write_backspace(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "cmd4");
  CHECK(app_execute_command_buffer(buf), CMD_ERR);


  CHECK(app_clear_command_buffer(buf), OK);
  CHECK_CMDBUF(buf, len, dump, "");
  CHECK(app_add_command
    (app, "foo__", foo, 
     1, APP_CMDARGV(APP_CMDARG_APPEND_STRING(cmd_option_list)), 
     NULL), OK);
  CHECK(app_command_buffer_completion(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_command_buffer_completion(buf, NULL, NULL), OK);
  CHECK_CMDBUF(buf, len, dump, "");
  CHECK(app_command_buffer_write_string(buf, "foo_"), OK);
  CHECK(app_command_buffer_completion(buf, &len2, &list), OK);
  CHECK(list, NULL);
  CHECK(len2, 0);
  CHECK_CMDBUF(buf, len, dump, "foo__");
  CHECK(app_command_buffer_completion(buf, &len2, &list), OK);
  CHECK_CMDBUF(buf, len, dump, "foo__");
  CHECK(list, NULL);
  CHECK(len2, 0);

  CHECK(app_command_buffer_write_char(buf, ' '), OK);
  CHECK(app_command_buffer_completion(buf, &len2, &list), OK);
  CHECK_CMDBUF(buf, len, dump, "foo__ ");
  CHECK(len2, 5);
  CHECK(list[0], "foo");
  CHECK(list[1], "opt");
  CHECK(list[2], "opt0");
  CHECK(list[3], "optionA");
  CHECK(list[4], "optionB");
  CHECK(app_command_buffer_write_string(buf, "op"), OK);
  CHECK(app_command_buffer_completion(buf, &len2, &list), OK);
  CHECK_CMDBUF(buf, len, dump, "foo__ opt");
  CHECK(list, NULL);
  CHECK(len2, 0);
  CHECK(app_command_buffer_completion(buf, &len2, &list), OK);
  CHECK_CMDBUF(buf, len, dump, "foo__ opt");
  CHECK(len2, 4);
  CHECK(list[0], "opt");
  CHECK(list[1], "opt0");
  CHECK(list[2], "optionA");
  CHECK(list[3], "optionB");
  CHECK(app_command_buffer_write_char(buf, '0'), OK);
  CHECK(app_command_buffer_completion(buf, &len2, &list), OK);
  CHECK_CMDBUF(buf, len, dump, "foo__ opt0");
  CHECK(len2, 0);
  CHECK(list, NULL);
  CHECK(app_command_buffer_completion(buf, &len2, &list), OK);
  CHECK_CMDBUF(buf, len, dump, "foo__ opt0");
  CHECK(len2, 0);
  CHECK(list, NULL);
  CHECK(app_command_buffer_move_cursor(buf, -1), OK);
  CHECK(app_command_buffer_write_char(buf, 'i'), OK);
  CHECK(app_command_buffer_completion(buf, &len2, &list), OK);
  CHECK_CMDBUF(buf, len, dump, "foo__ option0");
  CHECK(len2, 0);
  CHECK(list, NULL);
  CHECK(app_command_buffer_completion(buf, &len2, &list), OK);
  CHECK_CMDBUF(buf, len, dump, "foo__ option0");
  CHECK(len2, 2);
  CHECK(list[0], "optionA");
  CHECK(list[1], "optionB");
  CHECK(app_command_buffer_write_char(buf, 'A'), OK);
  CHECK(app_command_buffer_completion(buf, &len2, &list), OK);
  CHECK_CMDBUF(buf, len, dump, "foo__ optionA0");
  CHECK(len2, 0);
  CHECK(list, NULL);
  CHECK(app_command_buffer_completion(buf, &len2, &list), OK);
  CHECK_CMDBUF(buf, len, dump, "foo__ optionA0");
  CHECK(len2, 0);
  CHECK(list, NULL);
  
  CHECK(app_command_buffer_ref_get(NULL), BAD_ARG);
  CHECK(app_command_buffer_ref_get(buf), OK);
  CHECK(app_command_buffer_ref_put(NULL), BAD_ARG);
  CHECK(app_command_buffer_ref_put(buf), OK);
  CHECK(app_command_buffer_ref_put(buf), OK);

  CHECK(app_ref_put(app), OK);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}
