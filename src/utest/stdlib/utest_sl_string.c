#include "sys/mem_allocator.h"
#include "sys/sys.h"

#define SL_STRING_TYPE SL_STRING_STD
  #include "utest/stdlib/utest_sl_string_def.c"
#undef SL_STRING_TYPE

#define SL_STRING_TYPE SL_STRING_WIDE
  #include "utest/stdlib/utest_sl_string_def.c"
#undef SL_STRING_TYPE

int
main(int argc UNUSED, char** argv UNUSED)
{
  test_string();
  test_wstring();
}

