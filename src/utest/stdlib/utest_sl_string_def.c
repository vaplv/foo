#include "sys/sys.h"
#include "utest/utest.h"
#include <stdbool.h>
#include <stdlib.h>

#undef SL_STRING_STD
#undef SL_STRING_WIDE
#define SL_STRING_STD 0
#define SL_STRING_WIDE 1

#if (SL_STRING_TYPE == SL_STRING_STD)
  #include <string.h>
  #define CHAR_TYPE char
  #define CHAR(a) a
  #define STRING(str) str
  #define STRCMP strcmp
  #define STRLEN strlen 
#elif (SL_STRING_TYPE == SL_STRING_WIDE)
  #include <wchar.h>
  #define CHAR_TYPE wchar_t
  #define CHAR(a) CONCAT(L, a)
  #define STRING(str) CONCAT(L, str)
  #define STRCMP wcscmp
  #define STRLEN wcslen
#else
  #error "Unexpected string type."
#endif

#undef SL_STRING_STD
#undef SL_STRING_WIDE

/* Actually we should include the specific string (sl_string.h) or wide string
 * (sl_wstring.h) header file rather thant the generic string definition
 * header. However, we use the genericity mecanism defines into the
 * sl_string_def header to factorize the test of the wide and std string. */
#include "stdlib/sl_string_def.h"

#define TEST(type) CONCAT(test_, type)
#define BAD_ARG SL_INVALID_ARGUMENT
#define OK SL_NO_ERROR

static void
TEST(SL_STRING_TYPE)(void)
{
  SL_STRING(SL_STRING_TYPE)* str = NULL;
  const CHAR_TYPE* cstr = NULL;
  size_t i = 0;
  bool is_empty = false;

  CHECK(SL_CREATE_STRING(SL_STRING_TYPE)(NULL, NULL, NULL), BAD_ARG);
  CHECK(SL_CREATE_STRING(SL_STRING_TYPE)(STRING("Foo"), NULL, NULL), BAD_ARG);
  CHECK(SL_CREATE_STRING(SL_STRING_TYPE)(NULL, NULL, &str), OK);

  CHECK(SL_STRING_GET(SL_STRING_TYPE)(NULL, NULL), BAD_ARG);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, NULL), BAD_ARG);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(NULL, &cstr), BAD_ARG);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("")), 0);

  CHECK(SL_IS_STRING_EMPTY(SL_STRING_TYPE)(NULL, NULL), BAD_ARG);
  CHECK(SL_IS_STRING_EMPTY(SL_STRING_TYPE)(str, NULL), BAD_ARG);
  CHECK(SL_IS_STRING_EMPTY(SL_STRING_TYPE)(NULL,&is_empty), BAD_ARG);
  CHECK(SL_IS_STRING_EMPTY(SL_STRING_TYPE)(str, &is_empty), OK);
  CHECK(is_empty, true);

  CHECK(SL_FREE_STRING(SL_STRING_TYPE)(NULL), BAD_ARG);
  CHECK(SL_FREE_STRING(SL_STRING_TYPE)(str), OK);

  CHECK(SL_CREATE_STRING(SL_STRING_TYPE)(STRING("Foo"), NULL, &str), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("Foo")), 0);
  CHECK(SL_IS_STRING_EMPTY(SL_STRING_TYPE)(str, &is_empty), OK);
  CHECK(is_empty, false);

  CHECK(SL_CLEAR_STRING(SL_STRING_TYPE)(NULL), BAD_ARG);
  CHECK(SL_CLEAR_STRING(SL_STRING_TYPE)(str), OK);
  CHECK(SL_IS_STRING_EMPTY(SL_STRING_TYPE)(str, &is_empty), OK);
  CHECK(is_empty, true);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("")), 0);

  CHECK(SL_STRING_SET(SL_STRING_TYPE)(NULL, NULL), BAD_ARG);
  CHECK(SL_STRING_SET(SL_STRING_TYPE)(NULL, STRING(__FILE__)), BAD_ARG);
  CHECK(SL_STRING_SET(SL_STRING_TYPE)(str, NULL), BAD_ARG);
  CHECK(SL_STRING_SET(SL_STRING_TYPE)(str, STRING(__FILE__)), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING(__FILE__)), 0);

  CHECK(SL_STRING_SET(SL_STRING_TYPE)(str, STRING("Hello world!")), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("Hello world!")), 0);

  CHECK(SL_STRING_INSERT(SL_STRING_TYPE)(NULL, 13, NULL), BAD_ARG);
  CHECK(SL_STRING_INSERT(SL_STRING_TYPE)(str, 13, NULL), BAD_ARG);
  CHECK(SL_STRING_INSERT(SL_STRING_TYPE)(NULL, 13, STRING(" insert")), BAD_ARG);
  CHECK(SL_STRING_INSERT(SL_STRING_TYPE)(str, 13, STRING(" insert")), BAD_ARG);
  CHECK(SL_STRING_INSERT(SL_STRING_TYPE)(NULL, 12, NULL), BAD_ARG);
  CHECK(SL_STRING_INSERT(SL_STRING_TYPE)(str, 12, NULL), BAD_ARG);
  CHECK(SL_STRING_INSERT(SL_STRING_TYPE)(NULL, 12, STRING(" insert")), BAD_ARG);
  CHECK(SL_STRING_INSERT(SL_STRING_TYPE)(str, 12, STRING(" insert")), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("Hello world! insert")), 0);

  CHECK(SL_STRING_INSERT(SL_STRING_TYPE)(str, 6, STRING("abcdefgh ")), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("Hello abcdefgh world! insert")), 0);

  CHECK(SL_STRING_INSERT(SL_STRING_TYPE)(str, 0, STRING("ABC ")), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("ABC Hello abcdefgh world! insert")), 0);

  CHECK(SL_STRING_INSERT(SL_STRING_TYPE)(str, 11, STRING("0123456789ABCDEF")), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr,STRING("ABC Hello a0123456789ABCDEFbcdefgh world! insert")),0);

  CHECK(SL_STRING_LENGTH(SL_STRING_TYPE)(NULL, NULL), BAD_ARG);
  CHECK(SL_STRING_LENGTH(SL_STRING_TYPE)(str, NULL), BAD_ARG);
  CHECK(SL_STRING_LENGTH(SL_STRING_TYPE)(NULL, &i), BAD_ARG);
  CHECK(SL_STRING_LENGTH(SL_STRING_TYPE)(str, &i), OK);
  CHECK(i, STRLEN(cstr));

  CHECK(SL_STRING_SET(SL_STRING_TYPE)(str, STRING("Hello world!")), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(SL_STRING_INSERT(SL_STRING_TYPE)(str, 6, cstr), BAD_ARG);

  CHECK(SL_STRING_APPEND(SL_STRING_TYPE)(NULL, NULL), BAD_ARG);
  CHECK(SL_STRING_APPEND(SL_STRING_TYPE)(str, NULL), BAD_ARG);
  CHECK(SL_STRING_APPEND(SL_STRING_TYPE)(NULL, STRING(" Append")), BAD_ARG);
  CHECK(SL_STRING_APPEND(SL_STRING_TYPE)(str, STRING(" Append")), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("Hello world! Append")), 0);

  CHECK(SL_STRING_APPEND(SL_STRING_TYPE)(str, STRING("!")), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("Hello world! Append!")), 0);

  CHECK(SL_STRING_INSERT_CHAR(SL_STRING_TYPE)(NULL, 21, CHAR('a')), BAD_ARG);
  CHECK(SL_STRING_INSERT_CHAR(SL_STRING_TYPE)(str, 21, CHAR('a')), BAD_ARG);
  CHECK(SL_STRING_INSERT_CHAR(SL_STRING_TYPE)(NULL, 20, CHAR('a')), BAD_ARG);
  CHECK(SL_STRING_INSERT_CHAR(SL_STRING_TYPE)(str, 20, CHAR('a')), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("Hello world! Append!a")), 0);

  CHECK(SL_STRING_INSERT_CHAR(SL_STRING_TYPE)(str, 0, CHAR('0')), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("0Hello world! Append!a")), 0);
 
  CHECK(SL_STRING_INSERT_CHAR(SL_STRING_TYPE)(str, 13, CHAR('A')), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("0Hello world!A Append!a")), 0);

  CHECK(SL_STRING_APPEND_CHAR(SL_STRING_TYPE)(NULL, CHAR('z')), BAD_ARG);
  CHECK(SL_STRING_APPEND_CHAR(SL_STRING_TYPE)(str, CHAR('z')), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("0Hello world!A Append!az")), 0);

  CHECK(SL_STRING_LENGTH(SL_STRING_TYPE)(str, &i), OK);
  CHECK(i, STRLEN(cstr));

  CHECK(SL_STRING_ERASE(SL_STRING_TYPE)(NULL, i, 9), BAD_ARG);
  CHECK(SL_STRING_ERASE(SL_STRING_TYPE)(str, i, 9), BAD_ARG);
  CHECK(SL_STRING_ERASE(SL_STRING_TYPE)(NULL, 13, 9), BAD_ARG);
  CHECK(SL_STRING_ERASE(SL_STRING_TYPE)(str, 13, 9), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("0Hello world!az")), 0);
  CHECK(SL_STRING_LENGTH(SL_STRING_TYPE)(str, &i), OK);
  CHECK(i, STRLEN(cstr));

  CHECK(SL_STRING_ERASE(SL_STRING_TYPE)(str, 0, 1), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("Hello world!az")), 0);
  CHECK(SL_STRING_LENGTH(SL_STRING_TYPE)(str, &i), OK);
  CHECK(i, STRLEN(cstr));

  CHECK(SL_STRING_ERASE(SL_STRING_TYPE)(str, 12, SIZE_MAX), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("Hello world!")), 0);
  CHECK(SL_STRING_LENGTH(SL_STRING_TYPE)(str, &i), OK);
  CHECK(i, STRLEN(cstr));

  CHECK(SL_STRING_ERASE_CHAR(SL_STRING_TYPE)(NULL, 12), BAD_ARG);
  CHECK(SL_STRING_ERASE_CHAR(SL_STRING_TYPE)(str, 12), BAD_ARG);
  CHECK(SL_STRING_ERASE_CHAR(SL_STRING_TYPE)(NULL, 11), BAD_ARG);
  CHECK(SL_STRING_ERASE_CHAR(SL_STRING_TYPE)(str, 11), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("Hello world")), 0);
  CHECK(SL_STRING_LENGTH(SL_STRING_TYPE)(str, &i), OK);
  CHECK(i, STRLEN(cstr));

  CHECK(SL_STRING_ERASE_CHAR(SL_STRING_TYPE)(str, 0), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("ello world")), 0);
  CHECK(SL_STRING_LENGTH(SL_STRING_TYPE)(str, &i), OK);
  CHECK(i, STRLEN(cstr));

  CHECK(SL_STRING_ERASE_CHAR(SL_STRING_TYPE)(str, 5), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("ello orld")), 0);
  CHECK(SL_STRING_LENGTH(SL_STRING_TYPE)(str, &i), OK);
  CHECK(i, STRLEN(cstr));

  CHECK(SL_STRING_CAPACITY(SL_STRING_TYPE)(NULL, 0), BAD_ARG);
  CHECK(SL_STRING_CAPACITY(SL_STRING_TYPE)(str, 0), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("ello orld")), 0);

  CHECK(SL_STRING_LENGTH(SL_STRING_TYPE)(str, &i), OK);
  CHECK(i, STRLEN(cstr));

  CHECK(SL_STRING_CAPACITY(SL_STRING_TYPE)(str, 78), OK);
  CHECK(SL_STRING_GET(SL_STRING_TYPE)(str, &cstr), OK);
  CHECK(STRCMP(cstr, STRING("ello orld")), 0);
  CHECK(SL_STRING_LENGTH(SL_STRING_TYPE)(str, &i), OK);
  CHECK(i, STRLEN(cstr));

  CHECK(SL_FREE_STRING(SL_STRING_TYPE)(str), OK);
}

#undef BAD_ARG
#undef OK
#undef CHAR_TYPE
#undef CHAR
#undef STRING
#undef STRCMP
#undef STRLEN
