#include "app/core/app.h"
#include "app/core/app_command_buffer.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define BAD_ARG APP_INVALID_ARGUMENT
#define OK APP_NO_ERROR
#define BUFFER_SIZE 512

int
main(int argc, char** argv)
{
  char dump[BUFFER_SIZE] = { [0] = 'a', [1] = '\0' };
  struct app_args args = { NULL, NULL, NULL, NULL };
  struct app* app = NULL;
  struct app_command_buffer* buf = NULL;
  size_t len = SIZE_MAX;

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
  CHECK(len, 4);
  CHECK(strcmp(dump, "foo"), 0);

  CHECK(app_command_buffer_write_string(buf, ". Hello world"), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 17);
  CHECK(strcmp(dump, "foo. Hello world"), 0);

  CHECK(app_command_buffer_write_char(NULL, '!'), BAD_ARG);
  CHECK(app_command_buffer_write_char(buf, '!'), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 18);
  CHECK(strcmp(dump, "foo. Hello world!"), 0);

  CHECK(app_command_buffer_write_char(buf, '.'), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 19);
  CHECK(strcmp(dump, "foo. Hello world!."), 0);

  CHECK(app_command_buffer_write_backspace(NULL), BAD_ARG);
  CHECK(app_command_buffer_write_backspace(buf), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 18);
  CHECK(strcmp(dump, "foo. Hello world!"), 0);

  CHECK(app_command_buffer_write_suppr(NULL), BAD_ARG);
  CHECK(app_command_buffer_write_suppr(buf), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 18);
  CHECK(strcmp(dump, "foo. Hello world!"), 0);

  CHECK(app_command_buffer_move_cursor(NULL, 1), BAD_ARG);
  CHECK(app_command_buffer_move_cursor(buf, 1), OK);
  CHECK(app_command_buffer_write_string(buf, " :)"), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 21);
  CHECK(strcmp(dump, "foo. Hello world! :)"), 0);

  CHECK(app_command_buffer_move_cursor(buf, INT_MAX), OK);
  CHECK(app_command_buffer_write_string(buf, "..."), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 24);
  CHECK(strcmp(dump, "foo. Hello world! :)..."), 0);

  CHECK(app_command_buffer_move_cursor(buf, -4), OK);
  CHECK(app_command_buffer_write_char(buf, '-'), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 25);
  CHECK(strcmp(dump, "foo. Hello world! :-)..."), 0);

  CHECK(app_command_buffer_write_suppr(buf), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 24);
  CHECK(strcmp(dump, "foo. Hello world! :-..."), 0);

  CHECK(app_command_buffer_write_string(buf, "/!"), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 26);
  CHECK(strcmp(dump, "foo. Hello world! :-/!..."), 0);

  CHECK(app_command_buffer_move_cursor(buf, -18), OK);
  CHECK(app_command_buffer_write_backspace(buf), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 25);
  CHECK(strcmp(dump, "foo Hello world! :-/!..."), 0);

  CHECK(app_command_buffer_write_string(buf, "!!!!"), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 29);
  CHECK(strcmp(dump, "foo!!!! Hello world! :-/!..."), 0);

  CHECK(app_command_buffer_move_cursor(buf, INT_MIN), OK);
  CHECK(app_command_buffer_write_backspace(buf), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 29);
  CHECK(strcmp(dump, "foo!!!! Hello world! :-/!..."), 0);

  CHECK(app_command_buffer_write_suppr(buf), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 28);
  CHECK(strcmp(dump, "oo!!!! Hello world! :-/!..."), 0);

  CHECK(app_command_buffer_write_char(buf, 'F'), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 29);
  CHECK(strcmp(dump, "Foo!!!! Hello world! :-/!..."), 0);

  CHECK(app_clear_command_buffer(NULL), BAD_ARG);
  CHECK(app_clear_command_buffer(buf), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 0);
  CHECK(dump[0], '\0');

  CHECK(app_command_buffer_write_string(buf, "ls"), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 3);
  CHECK(strcmp(dump, "ls"), 0);

  CHECK(app_command_buffer_move_cursor(buf, INT_MIN), OK);
  CHECK(app_command_buffer_write_string(buf, "help "), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 8);
  CHECK(strcmp(dump, "help ls"), 0);

  CHECK(app_execute_command_buffer(NULL), BAD_ARG);
  CHECK(app_execute_command_buffer(buf), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(len, 0);
  CHECK(dump[0], '\0');
  CHECK(app_execute_command_buffer(buf), OK);

  CHECK(app_command_buffer_write_string(buf, "wrong_command"), OK);
  CHECK(app_dump_command_buffer(buf, &len, dump), OK);
  CHECK(strcmp(dump, "wrong_command"), 0);
  CHECK(app_execute_command_buffer(buf), BAD_ARG);

  CHECK(app_command_buffer_ref_get(NULL), BAD_ARG);
  CHECK(app_command_buffer_ref_get(buf), OK);
  CHECK(app_command_buffer_ref_put(NULL), BAD_ARG);
  CHECK(app_command_buffer_ref_put(buf), OK);
  CHECK(app_command_buffer_ref_put(buf), OK);

  CHECK(app_ref_put(app), OK);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}
