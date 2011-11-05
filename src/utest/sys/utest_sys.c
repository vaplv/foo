#include "sys/sys.h"
#include "sys/sys_logger.h"
#include "utest/utest.h"
#include <string.h>

#define BAD_ARG SYS_INVALID_ARGUMENT
#define OK SYS_NO_ERROR

struct data {
  float f;
  char c;
};

static int func_mask = 0;

static void 
stream_func0(const char* msg, void* data)
{
  int* i = data;
  func_mask ^= 1;
  CHECK(*i, (int)0xDEADBEEF);
  CHECK(strcmp(msg, "logger 0"), 0);
}

static void 
stream_func1(const char* msg, void* data)
{
  struct data* d = data;
  func_mask ^= 2;
  CHECK(d->f, 3.14159f);
  CHECK(d->c, 'a');
  CHECK(strcmp(msg, "logger 0"), 0);
}

static void
stream_func2(const char* msg, void* data)
{
  func_mask ^= 4;
  CHECK(data, NULL);
  CHECK(strcmp(msg, "logger 0"), 0);
}

int
main(int argc UNUSED, char** argv UNUSED)
{
  struct sys* sys = NULL;
  struct sys_logger* logger = NULL;
  struct sys_log_stream stream = { NULL, NULL };
  struct data data = { 0, '\0' };

  CHECK(sys_init(NULL), BAD_ARG);
  CHECK(sys_init(&sys), OK);

  CHECK(sys_create_logger(NULL, NULL), BAD_ARG);
  CHECK(sys_create_logger(sys, NULL), BAD_ARG);
  CHECK(sys_create_logger(NULL, &logger), BAD_ARG);
  CHECK(sys_create_logger(sys, &logger), OK);

  stream.data = (int[]){0xDEADBEEF};
  stream.func = stream_func0;
  CHECK(sys_logger_add_stream(NULL, NULL, NULL), BAD_ARG);
  CHECK(sys_logger_add_stream(sys, NULL, NULL), BAD_ARG);
  CHECK(sys_logger_add_stream(NULL, logger, NULL), BAD_ARG);
  CHECK(sys_logger_add_stream(sys, logger, NULL), BAD_ARG);
  CHECK(sys_logger_add_stream(NULL, NULL, &stream), BAD_ARG);
  CHECK(sys_logger_add_stream(sys, NULL, &stream), BAD_ARG);
  CHECK(sys_logger_add_stream(NULL, logger, &stream), BAD_ARG);
  CHECK(sys_logger_add_stream(sys, logger, &stream), OK);
  CHECK(sys_logger_add_stream(sys, logger, &stream), BAD_ARG);

  CHECK(sys_logger_remove_stream(NULL, NULL, NULL), BAD_ARG);
  CHECK(sys_logger_remove_stream(sys, NULL, NULL), BAD_ARG);
  CHECK(sys_logger_remove_stream(NULL, logger, NULL), BAD_ARG);
  CHECK(sys_logger_remove_stream(sys, logger, NULL), BAD_ARG);
  CHECK(sys_logger_remove_stream(NULL, NULL, &stream), BAD_ARG);
  CHECK(sys_logger_remove_stream(sys, NULL, &stream), BAD_ARG);
  CHECK(sys_logger_remove_stream(NULL, logger, &stream), BAD_ARG);
  CHECK(sys_logger_remove_stream(sys, logger, &stream), OK);
  CHECK(sys_logger_remove_stream(sys, logger, &stream), BAD_ARG);

  CHECK(sys_logger_add_stream(sys, logger, &stream), OK);
  data.f = 3.14159f;
  data.c = 'a';
  stream.data = &data;
  stream.func = stream_func1;
  CHECK(sys_logger_add_stream(sys, logger, &stream), OK);
  CHECK(sys_logger_add_stream(sys, logger, &stream), BAD_ARG);
  stream.data = NULL;
  stream.func = stream_func2;
  CHECK(sys_logger_add_stream(sys, logger, &stream), OK);

  CHECK(sys_logger_print(NULL, NULL, NULL), BAD_ARG);
  CHECK(sys_logger_print(sys, NULL, NULL), BAD_ARG);
  CHECK(sys_logger_print(NULL, logger, NULL), BAD_ARG);
  CHECK(sys_logger_print(sys, logger, NULL), BAD_ARG);
  CHECK(sys_logger_print(NULL, NULL, "logger 0"), BAD_ARG);
  CHECK(sys_logger_print(sys, NULL, "logger 0"), BAD_ARG);
  CHECK(sys_logger_print(NULL, logger, "logger 0"), BAD_ARG);
  CHECK(sys_logger_print(sys, logger, "logger 0"), OK);
  CHECK(func_mask, 7);
  CHECK(sys_logger_print(sys, logger, "%c%s %d", 'l', "ogger", 0), OK);
  CHECK(func_mask, 0);
  CHECK(sys_logger_remove_stream(sys, logger, &stream), OK);
  CHECK(sys_logger_print(sys, logger, "logger 0"), OK);
  CHECK(func_mask, 3);
  data.f = 3.14159f;
  data.c = 'a';
  stream.data = &data;
  stream.func = stream_func1;
  CHECK(sys_logger_remove_stream(sys, logger, &stream), OK);
  CHECK(sys_logger_print(sys, logger, "%s%s 0", "log", "ger"), OK);
  CHECK(func_mask, 2);
  stream.data = (int[]){0xDEADBEEF};
  stream.func = stream_func0;
  CHECK(sys_logger_remove_stream(sys, logger, &stream), OK);
  CHECK(sys_logger_print(sys, logger, "pouet"), OK);
  CHECK(func_mask, 2);

  CHECK(sys_logger_add_stream(sys, logger, &stream), OK);
  data.f = 3.14159f;
  data.c = 'a';
  stream.data = &data;
  stream.func = stream_func1;
  CHECK(sys_logger_add_stream(sys, logger, &stream), OK);
  CHECK(sys_logger_add_stream(sys, logger, &stream), BAD_ARG);
  stream.data = NULL;
  stream.func = stream_func2;
  CHECK(sys_logger_add_stream(sys, logger, &stream), OK);
  func_mask = 0;
  CHECK(sys_logger_print(sys, logger, "logger 0"), OK);
  CHECK(func_mask, 7);

  CHECK(sys_clear_logger(NULL, NULL), BAD_ARG);
  CHECK(sys_clear_logger(sys, NULL), BAD_ARG);
  CHECK(sys_clear_logger(NULL, logger), BAD_ARG);
  CHECK(sys_clear_logger(sys, logger), OK);
  CHECK(sys_logger_print(sys, logger, "Awesom-o"), OK);
  CHECK(func_mask, 7);

  CHECK(sys_free_logger(NULL, NULL), BAD_ARG);
  CHECK(sys_free_logger(sys, NULL), BAD_ARG);
  CHECK(sys_free_logger(NULL, logger), BAD_ARG);
  CHECK(sys_free_logger(sys, logger), OK);
  CHECK(sys_shutdown(NULL), BAD_ARG);
  CHECK(sys_shutdown(sys), OK);

  return 0;
}

