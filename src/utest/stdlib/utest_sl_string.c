#include "stdlib/sl_string.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "utest/utest.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc UNUSED, char** argv UNUSED)
{
  struct sl_string* str = NULL;
  const char* cstr = NULL;
  bool is_empty = false;

  CHECK(sl_create_string(NULL, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_create_string("Foo", NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_create_string(NULL, NULL, &str), SL_NO_ERROR);

  CHECK(sl_string_get(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_string_get(str, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_string_get(NULL, &cstr), SL_INVALID_ARGUMENT);
  CHECK(sl_string_get(str, &cstr), SL_NO_ERROR);
  CHECK(strcmp(cstr, ""), 0);

  CHECK(sl_is_string_empty(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_is_string_empty(str, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_is_string_empty(NULL,&is_empty), SL_INVALID_ARGUMENT);
  CHECK(sl_is_string_empty(str, &is_empty), SL_NO_ERROR);
  CHECK(is_empty, true);

  CHECK(sl_free_string(NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_free_string(str), SL_NO_ERROR);

  CHECK(sl_create_string("Foo", NULL, &str), SL_NO_ERROR);
  CHECK(sl_string_get(str, &cstr), SL_NO_ERROR);
  CHECK(strcmp(cstr, "Foo"), 0);
  CHECK(sl_is_string_empty(str, &is_empty), SL_NO_ERROR);
  CHECK(is_empty, false);

  CHECK(sl_clear_string(NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_clear_string(str), SL_NO_ERROR);
  CHECK(sl_is_string_empty(str, &is_empty), SL_NO_ERROR);
  CHECK(is_empty, true);
  CHECK(sl_string_get(str, &cstr), SL_NO_ERROR);
  CHECK(strcmp(cstr, ""), 0);

  CHECK(sl_string_set(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_string_set(NULL, __FILE__), SL_INVALID_ARGUMENT);
  CHECK(sl_string_set(str, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_string_set(str, __FILE__), SL_NO_ERROR);
  CHECK(sl_string_get(str, &cstr), SL_NO_ERROR);
  CHECK(strcmp(cstr, __FILE__), 0);

  CHECK(sl_string_set(str, "Hello world!"), SL_NO_ERROR);
  CHECK(sl_string_get(str, &cstr), SL_NO_ERROR);
  CHECK(strcmp(cstr, "Hello world!"), 0);

  CHECK(sl_free_string(str), SL_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}

