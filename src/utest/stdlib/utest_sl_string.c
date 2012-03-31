#include "sys/mem_allocator.h"
#include "sys/sys.h"

#define SL_STRING_TYPE SL_STRING_STD
  #include "utest/stdlib/utest_sl_string_def.c"
#undef SL_STRING_TYPE

#define SL_STRING_TYPE SL_STRING_WIDE
  #include "utest/stdlib/utest_sl_string_def.c"
#undef SL_STRING_TYPE

#include "stdlib/sl_wstring.h"

static void
test_wstring_specific_func()
{
  struct sl_wstring* str = NULL;
  const wchar_t* cstr = NULL;

  CHECK(sl_create_wstring(L"Heo!", NULL, &str), SL_NO_ERROR);

  CHECK(sl_wstring_insert_cstr(NULL, 2, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_wstring_insert_cstr(str, 2, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_wstring_insert_cstr(NULL, 2, "ll"), SL_INVALID_ARGUMENT);
  CHECK(sl_wstring_insert_cstr(str, 2, "ll"), SL_NO_ERROR);
  CHECK(sl_wstring_get(str, &cstr), SL_NO_ERROR);
  CHECK(wcscmp(cstr, L"Hello!"), 0);

  CHECK(sl_wstring_insert_cstr(str, 5, " wrld"), SL_NO_ERROR);
  CHECK(sl_wstring_get(str, &cstr), SL_NO_ERROR);
  CHECK(wcscmp(cstr, L"Hello wrld!"), 0);

  CHECK(sl_wstring_insert_wchar(str, 7, (wchar_t)'o'), SL_NO_ERROR);
  CHECK(sl_wstring_get(str, &cstr), SL_NO_ERROR);
  CHECK(wcscmp(cstr, L"Hello world!"), 0);

  CHECK(sl_wstring_insert_cstr(str, 13, "..."), SL_INVALID_ARGUMENT);
  CHECK(sl_wstring_insert_cstr(str, 12, "..."), SL_NO_ERROR);
  CHECK(sl_wstring_get(str, &cstr), SL_NO_ERROR);
  CHECK(wcscmp(cstr, L"Hello world!..."), 0);

  CHECK(sl_wstring_insert_cstr(str, 0, "Foo. "), SL_NO_ERROR);
  CHECK(sl_wstring_get(str, &cstr), SL_NO_ERROR);
  CHECK(wcscmp(cstr, L"Foo. Hello world!..."), 0);

  CHECK(sl_wstring_append_cstr(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_wstring_append_cstr(str, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_wstring_append_cstr(NULL, " abcd"), SL_INVALID_ARGUMENT);
  CHECK(sl_wstring_append_cstr(str, " abcd"), SL_NO_ERROR);
  CHECK(sl_wstring_get(str, &cstr), SL_NO_ERROR);
  CHECK(wcscmp(cstr, L"Foo. Hello world!... abcd"), 0);

  CHECK(sl_free_wstring(str), SL_NO_ERROR);
}

int
main(int argc UNUSED, char** argv UNUSED)
{
  test_string();
  test_wstring();
  test_wstring_specific_func();

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}

