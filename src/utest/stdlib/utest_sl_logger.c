#include "stdlib/sl_logger.h"
#include "utest/utest.h"
#include <string.h>

#define BAD_ARG SL_INVALID_ARGUMENT
#define OK SL_NO_ERROR

STATIC_ASSERT(sizeof(void*) >= 4, Unexpected_pointer_size);

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
  CHECK((uintptr_t)i, 0xDEADBEEF);
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
  struct sl_logger* logger = NULL;
  struct sl_log_stream stream = { NULL, NULL };
  struct data data = { 0, '\0' };

  CHECK(sl_create_logger(NULL, NULL), BAD_ARG);
  CHECK(sl_create_logger(NULL, &logger), OK);

  stream.data = (void*)0xDEADBEEF;
  stream.func = stream_func0;
  CHECK(sl_logger_add_stream(NULL, NULL), BAD_ARG);
  CHECK(sl_logger_add_stream(logger, NULL), BAD_ARG);
  CHECK(sl_logger_add_stream(NULL, &stream), BAD_ARG);
  CHECK(sl_logger_add_stream(logger, &stream), OK);
  CHECK(sl_logger_add_stream(logger, &stream), BAD_ARG);

  CHECK(sl_logger_remove_stream(NULL, NULL), BAD_ARG);
  CHECK(sl_logger_remove_stream(logger, NULL), BAD_ARG);
  CHECK(sl_logger_remove_stream(NULL, &stream), BAD_ARG);
  CHECK(sl_logger_remove_stream(logger, &stream), OK);
  CHECK(sl_logger_remove_stream(logger, &stream), BAD_ARG);

  CHECK(sl_logger_add_stream(logger, &stream), OK);
  data.f = 3.14159f;
  data.c = 'a';
  stream.data = &data;
  stream.func = stream_func1;
  CHECK(sl_logger_add_stream(logger, &stream), OK);
  CHECK(sl_logger_add_stream(logger, &stream), BAD_ARG);
  stream.data = NULL;
  stream.func = stream_func2;
  CHECK(sl_logger_add_stream(logger, &stream), OK);

  CHECK(sl_logger_print(NULL, NULL), BAD_ARG);
  CHECK(sl_logger_print(logger, NULL), BAD_ARG);
  CHECK(sl_logger_print(NULL, "logger 0"), BAD_ARG);
  CHECK(sl_logger_print(logger, "logger 0"), OK);
  CHECK(func_mask, 7);
  CHECK(sl_logger_print(logger, "%c%s %d", 'l', "ogger", 0), OK);
  CHECK(func_mask, 0);
  CHECK(sl_logger_remove_stream(logger, &stream), OK);
  CHECK(sl_logger_print(logger, "logger 0"), OK);
  CHECK(func_mask, 3);
  data.f = 3.14159f;
  data.c = 'a';
  stream.data = &data;
  stream.func = stream_func1;
  CHECK(sl_logger_remove_stream(logger, &stream), OK);
  CHECK(sl_logger_print(logger, "%s%s 0", "log", "ger"), OK);
  CHECK(func_mask, 2);
  stream.data = (void*)0xDEADBEEF;
  stream.func = stream_func0;
  CHECK(sl_logger_remove_stream(logger, &stream), OK);
  CHECK(sl_logger_print(logger, "pouet"), OK);
  CHECK(func_mask, 2);

  CHECK(sl_logger_add_stream(logger, &stream), OK);
  data.f = 3.14159f;
  data.c = 'a';
  stream.data = &data;
  stream.func = stream_func1;
  CHECK(sl_logger_add_stream(logger, &stream), OK);
  CHECK(sl_logger_add_stream(logger, &stream), BAD_ARG);
  stream.data = NULL;
  stream.func = stream_func2;
  CHECK(sl_logger_add_stream(logger, &stream), OK);
  func_mask = 0;
  CHECK(sl_logger_print(logger, "logger 0"), OK);
  CHECK(func_mask, 7);

  CHECK(sl_clear_logger(NULL), BAD_ARG);
  CHECK(sl_clear_logger(logger), OK);
  CHECK(sl_logger_print(logger, "Awesom-o"), OK);
  CHECK(func_mask, 7);

  CHECK(sl_free_logger(NULL), BAD_ARG);
  CHECK(sl_free_logger(logger), OK);

  return 0;
}

