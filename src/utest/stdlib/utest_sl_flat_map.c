#include "stdlib/sl_flat_map.h"
#include "stdlib/sl_pair.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "utest/utest.h"
#include <stdbool.h>
#include <stddef.h>

#define BAD_ARG SL_INVALID_ARGUMENT
#define BAD_AL SL_ALIGNMENT_ERROR
#define OK SL_NO_ERROR
#define SZK sizeof(int)
#define ALK ALIGNOF(int)
#define SZD sizeof(char)
#define ALD ALIGNOF(char)

static int
cmp(const void* p0, const void* p1)
{
  return *((const int*)p0) - *((const int*)p1);
}

int
main(int argc UNUSED, char** argv UNUSED)
{
  ALIGN(16) int array[2] = {0, 1};
  void* ptr = NULL;
  void* buffer = NULL;
  struct sl_flat_map* map = NULL;
  struct sl_pair pair = { NULL, NULL };
  size_t al = 0;
  size_t i = 0;
  size_t len = 0;
  size_t sz = 0;

  STATIC_ASSERT(!IS_ALIGNED(&array[1], 16), Unexpected_alignment);

  CHECK(sl_create_flat_map(0, 0, 0, 0, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, 0, 0, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, 0, 0, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, 0, 0, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, SZD, 0, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, SZD, 0, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, SZD, 0, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, SZD, 0, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, 0, ALD, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, 0, ALD, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, 0, ALD, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, 0, ALD, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, SZD, ALD, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, SZD, ALD, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, SZD, ALD, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, SZD, ALD, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, 0, 0, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, 0, 0, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, 0, 0, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, 0, 0, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, SZD, 0, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, SZD, 0, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, SZD, 0, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, SZD, 0, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, 0, ALD, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, 0, ALD, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, 0, ALD, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, 0, ALD, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, SZD, ALD, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, SZD, ALD, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, SZD, ALD, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, SZD, ALD, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, 0, 0, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, 0, 0, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, 0, 0, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, 0, 0, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, SZD, 0, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, SZD, 0, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, SZD, 0, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, SZD, 0, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, 0, ALD, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, 0, ALD, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, 0, ALD, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, 0, ALD, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, SZD, ALD, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, SZD, ALD, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, SZD, ALD, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, SZD, ALD, NULL, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, 0, 0, cmp, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, 0, 0, cmp, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, 0, 0, cmp, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, 0, 0, cmp, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, SZD, 0, cmp, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, SZD, 0, cmp, NULL, &map), BAD_AL);
  CHECK(sl_create_flat_map(0, ALK, SZD, 0, cmp, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, SZD, 0, cmp, NULL, &map), BAD_AL);
  CHECK(sl_create_flat_map(0, 0, 0, ALD, cmp, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, 0, ALD, cmp, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(0, ALK, 0, ALD, cmp, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, 0, ALD, cmp, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(0, 0, SZD, ALD, cmp, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, 0, SZD, ALD, cmp, NULL, &map), BAD_AL);
  CHECK(sl_create_flat_map(0, ALK, SZD, ALD, cmp, NULL, &map), BAD_ARG);
  CHECK(sl_create_flat_map(SZK, ALK, SZD, ALD, cmp, NULL, &map), OK);

  CHECK(sl_flat_map_insert(NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_insert(map, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_insert(NULL, (int[]){0}, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_insert(map, (int[]){0}, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_insert(NULL, NULL, (char[]){'a'}, NULL), BAD_ARG);
  CHECK(sl_flat_map_insert(map, NULL, (char[]){'a'}, NULL), BAD_ARG);
  CHECK(sl_flat_map_insert(NULL, (int[]){0}, (char[]){'a'}, NULL), BAD_ARG);
  CHECK(sl_flat_map_insert(map, (int[]){0}, (char[]){'a'}, NULL), OK);

  CHECK(sl_flat_map_length(NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_length(map, NULL), BAD_ARG);
  CHECK(sl_flat_map_length(NULL, &len), BAD_ARG);
  CHECK(sl_flat_map_length(map, &len), OK);
  CHECK(len, 1);
  CHECK(sl_flat_map_insert(map, (int[]){0}, (char[]){'a'}, NULL), BAD_ARG);

  CHECK(sl_flat_map_erase(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_erase(map, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_erase(NULL, (int[]){0}, NULL), BAD_ARG);
  CHECK(sl_flat_map_erase(map, (int[]){0}, NULL), OK);
  CHECK(sl_flat_map_erase(map, (int[]){0}, NULL), BAD_ARG);

  CHECK(sl_flat_map_insert(map, (int[]){2}, (char[]){'c'}, &i), OK);
  CHECK(i, 0);
  CHECK(sl_flat_map_insert(map, (int[]){1}, (char[]){'b'}, &i), OK);
  CHECK(i, 0);
  CHECK(sl_flat_map_insert(map, (int[]){0}, (char[]){'a'}, &i), OK);
  CHECK(i, 0);
  CHECK(sl_flat_map_insert(map, (int[]){3}, (char[]){'d'}, &i), OK);
  CHECK(i, 3);
  CHECK(sl_flat_map_length(map, &len), OK);
  CHECK(len, 4);

  CHECK(sl_flat_map_key_buffer(NULL, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_key_buffer(map, NULL, NULL, NULL, NULL), OK);
  CHECK(sl_flat_map_key_buffer(NULL, &len, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_key_buffer(map, &len, NULL, NULL, NULL), OK);
  CHECK(len, 4);
  CHECK(sl_flat_map_key_buffer(NULL, NULL, &sz, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_key_buffer(map, &len, &sz, NULL, NULL), OK);
  CHECK(len, 4);
  CHECK(sz, SZK);
  CHECK(sl_flat_map_key_buffer(map, NULL, &sz, NULL, NULL), OK);
  CHECK(sz, SZK);
  CHECK(sl_flat_map_key_buffer(NULL, NULL, NULL, &al, NULL), BAD_ARG);
  CHECK(sl_flat_map_key_buffer(map, NULL, NULL, &al, NULL), OK);
  CHECK(al, ALK);
  CHECK(sl_flat_map_key_buffer(map, &len, NULL, &al, NULL), OK);
  CHECK(len, 4);
  CHECK(al, ALK);
  CHECK(sl_flat_map_key_buffer(map, NULL, &sz, &al, NULL), OK);
  CHECK(sz, SZK);
  CHECK(al, ALK);
  CHECK(sl_flat_map_key_buffer(map, &len, &sz, &al, NULL), OK);
  CHECK(len, 4);
  CHECK(sz, SZK);
  CHECK(al, ALK);
  CHECK(sl_flat_map_key_buffer(map, &len, NULL, NULL, &buffer), OK);
  CHECK(len, 4);
  CHECK(((int*)buffer)[0], 0);
  CHECK(((int*)buffer)[1], 1);
  CHECK(((int*)buffer)[2], 2);
  CHECK(((int*)buffer)[3], 3);

  CHECK(sl_flat_map_data_buffer(NULL, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_data_buffer(map, NULL, NULL, NULL, NULL), OK);
  CHECK(sl_flat_map_data_buffer(NULL, &len, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_data_buffer(map, &len, NULL, NULL, NULL), OK);
  CHECK(len, 4);
  CHECK(sl_flat_map_data_buffer(NULL, NULL, &sz, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_data_buffer(map, &len, &sz, NULL, NULL), OK);
  CHECK(len, 4);
  CHECK(sz, SZD);
  CHECK(sl_flat_map_data_buffer(map, NULL, &sz, NULL, NULL), OK);
  CHECK(sz, SZD);
  CHECK(sl_flat_map_data_buffer(NULL, NULL, NULL, &al, NULL), BAD_ARG);
  CHECK(sl_flat_map_data_buffer(map, NULL, NULL, &al, NULL), OK);
  CHECK(al, ALD);
  CHECK(sl_flat_map_data_buffer(map, &len, NULL, &al, NULL), OK);
  CHECK(len, 4);
  CHECK(al, ALD);
  CHECK(sl_flat_map_data_buffer(map, NULL, &sz, &al, NULL), OK);
  CHECK(sz, SZD);
  CHECK(al, ALD);
  CHECK(sl_flat_map_data_buffer(map, &len, &sz, &al, NULL), OK);
  CHECK(len, 4);
  CHECK(sz, SZD);
  CHECK(al, ALD);
  CHECK(sl_flat_map_data_buffer(map, &len, NULL, NULL, &buffer), OK);
  CHECK(len, 4);
  CHECK(((char*)buffer)[0], 'a');
  CHECK(((char*)buffer)[1], 'b');
  CHECK(((char*)buffer)[2], 'c');
  CHECK(((char*)buffer)[3], 'd');

  CHECK(sl_flat_map_find(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_find(map, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_find(NULL, (int[]){0}, NULL), BAD_ARG);
  CHECK(sl_flat_map_find(map, (int[]){0}, NULL), BAD_ARG);
  CHECK(sl_flat_map_find(NULL, NULL, &ptr), BAD_ARG);
  CHECK(sl_flat_map_find(map, NULL, &ptr), BAD_ARG);
  CHECK(sl_flat_map_find(NULL, (int[]){0}, &ptr), BAD_ARG);
  CHECK(sl_flat_map_find(map, (int[]){0}, &ptr), OK);
  NCHECK(ptr, NULL);
  CHECK(*(char*)ptr, 'a');
  CHECK(sl_flat_map_find(map, (int[]){1}, &ptr), OK);
  NCHECK(ptr, NULL);
  CHECK(*(char*)ptr, 'b');
  CHECK(sl_flat_map_find(map, (int[]){2}, &ptr), OK);
  NCHECK(ptr, NULL);
  CHECK(*(char*)ptr, 'c');
  CHECK(sl_flat_map_find(map, (int[]){3}, &ptr), OK);
  NCHECK(ptr, NULL);
  CHECK(*(char*)ptr, 'd');
  CHECK(sl_flat_map_find(map, (int[]){4}, &ptr), OK);
  CHECK(ptr, NULL);

  CHECK(sl_flat_map_find_pair(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_find_pair(map, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_find_pair(NULL, (int[]){0}, NULL), BAD_ARG);
  CHECK(sl_flat_map_find_pair(map, (int[]){0}, NULL), BAD_ARG);
  CHECK(sl_flat_map_find_pair(NULL, NULL, &pair), BAD_ARG);
  CHECK(sl_flat_map_find_pair(map, NULL, &pair), BAD_ARG);
  CHECK(sl_flat_map_find_pair(NULL, (int[]){0}, &pair), BAD_ARG);
  CHECK(sl_flat_map_find_pair(map, (int[]){0}, &pair), OK);
  CHECK(*(int*)pair.key, 0);
  CHECK(*(char*)pair.data, 'a');
  CHECK(sl_flat_map_find_pair(map, (int[]){1}, &pair), OK);
  CHECK(*(int*)pair.key, 1);
  CHECK(*(char*)pair.data, 'b');
  CHECK(sl_flat_map_find_pair(map, (int[]){2}, &pair), OK);
  CHECK(*(int*)pair.key, 2);
  CHECK(*(char*)pair.data, 'c');
  CHECK(sl_flat_map_find_pair(map, (int[]){3}, &pair), OK);
  CHECK(*(int*)pair.key, 3);
  CHECK(*(char*)pair.data, 'd');

  CHECK(sl_clear_flat_map(NULL), BAD_ARG);
  CHECK(sl_clear_flat_map(map), OK);
  CHECK(sl_flat_map_length(map, &len), OK);
  CHECK(len, 0);
  CHECK(sl_flat_map_key_buffer(map, &len, NULL, NULL, &buffer), OK);
  CHECK(len, 0);
  CHECK(sl_flat_map_data_buffer(map, &len, NULL, NULL, &buffer), OK);
  CHECK(len, 0);

  CHECK(sl_flat_map_insert(map, (int[]){2}, (char[]){'c'}, &i), OK);
  CHECK(sl_flat_map_insert(map, (int[]){0}, (char[]){'a'}, &i), OK);
  CHECK(sl_flat_map_insert(map, (int[]){3}, (char[]){'d'}, &i), OK);
  CHECK(sl_flat_map_insert(map, (int[]){25}, (char[]){'z'}, &i), OK);
  CHECK(sl_flat_map_insert(map, (int[]){23}, (char[]){'x'}, &i), OK);
  CHECK(sl_flat_map_insert(map, (int[]){4}, (char[]){'e'}, &i), OK);
  CHECK(sl_flat_map_length(map, &len), OK);
  CHECK(len, 6);
  CHECK(sl_flat_map_data_buffer(map, &len, NULL, NULL, &buffer), OK);
  CHECK(len, 6);
  CHECK(((char*)buffer)[0], 'a');
  CHECK(((char*)buffer)[1], 'c');
  CHECK(((char*)buffer)[2], 'd');
  CHECK(((char*)buffer)[3], 'e');
  CHECK(((char*)buffer)[4], 'x');
  CHECK(((char*)buffer)[5], 'z');
  CHECK(sl_flat_map_key_buffer(map, &len, NULL, NULL, &buffer), OK);
  CHECK(len, 6);
  CHECK(((int*)buffer)[0], 0);
  CHECK(((int*)buffer)[1], 2);
  CHECK(((int*)buffer)[2], 3);
  CHECK(((int*)buffer)[3], 4);
  CHECK(((int*)buffer)[4], 23);
  CHECK(((int*)buffer)[5], 25);

  CHECK(sl_flat_map_at(NULL, 6, NULL), BAD_ARG);
  CHECK(sl_flat_map_at(map, 6, NULL), BAD_ARG);
  CHECK(sl_flat_map_at(NULL, 0, NULL), BAD_ARG);
  CHECK(sl_flat_map_at(map, 0, NULL), BAD_ARG);
  CHECK(sl_flat_map_at(NULL, 6, &pair), BAD_ARG);
  CHECK(sl_flat_map_at(map, 6, &pair), BAD_ARG);
  CHECK(sl_flat_map_at(NULL, 0, &pair), BAD_ARG);
  CHECK(sl_flat_map_at(map, 0, &pair), OK);
  CHECK(*((int*)pair.key), 0);
  CHECK(*((char*)pair.data), 'a');
  CHECK(sl_flat_map_at(map, 1, &pair), OK);
  CHECK(*((int*)pair.key), 2);
  CHECK(*((char*)pair.data), 'c');
  CHECK(sl_flat_map_at(map, 2, &pair), OK);
  CHECK(*((int*)pair.key), 3);
  CHECK(*((char*)pair.data), 'd');
  CHECK(sl_flat_map_at(map, 3, &pair), OK);
  CHECK(*((int*)pair.key), 4);
  CHECK(*((char*)pair.data), 'e');
  CHECK(sl_flat_map_at(map, 4, &pair), OK);
  CHECK(*((int*)pair.key), 23);
  CHECK(*((char*)pair.data), 'x');
  CHECK(sl_flat_map_at(map, 5, &pair), OK);
  CHECK(*((int*)pair.key), 25);
  CHECK(*((char*)pair.data), 'z');

  CHECK(sl_flat_map_lower_bound(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_lower_bound(map, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_lower_bound(NULL, (int[]){1}, NULL), BAD_ARG);
  CHECK(sl_flat_map_lower_bound(map, (int[]){1}, NULL), BAD_ARG);
  CHECK(sl_flat_map_lower_bound(NULL, NULL, &i), BAD_ARG);
  CHECK(sl_flat_map_lower_bound(map, NULL, &i), BAD_ARG);
  CHECK(sl_flat_map_lower_bound(NULL, (int[]){1}, &i), BAD_ARG);
  CHECK(sl_flat_map_lower_bound(map, (int[]){1}, &i), OK);
  CHECK(i, 1);
  CHECK(sl_flat_map_lower_bound(map, (int[]){0}, &i), OK);
  CHECK(i, 0);
  CHECK(sl_flat_map_lower_bound(map, (int[]){-1}, &i), OK);
  CHECK(i, 0);
  CHECK(sl_flat_map_lower_bound(map, (int[]){5}, &i), OK);
  CHECK(i, 4);
  CHECK(sl_flat_map_lower_bound(map, (int[]){24}, &i), OK);
  CHECK(i, 5);
  CHECK(sl_flat_map_lower_bound(map, (int[]){25}, &i), OK);
  CHECK(i, 5);
  CHECK(sl_flat_map_lower_bound(map, (int[]){26}, &i), OK);
  CHECK(i, 6);

  CHECK(sl_flat_map_upper_bound(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_upper_bound(map, NULL, NULL), BAD_ARG);
  CHECK(sl_flat_map_upper_bound(NULL, (int[]){1}, NULL), BAD_ARG);
  CHECK(sl_flat_map_upper_bound(map, (int[]){1}, NULL), BAD_ARG);
  CHECK(sl_flat_map_upper_bound(NULL, NULL, &i), BAD_ARG);
  CHECK(sl_flat_map_upper_bound(map, NULL, &i), BAD_ARG);
  CHECK(sl_flat_map_upper_bound(NULL, (int[]){1}, &i), BAD_ARG);
  CHECK(sl_flat_map_upper_bound(map, (int[]){1}, &i), OK);
  CHECK(i, 1);
  CHECK(sl_flat_map_upper_bound(map, (int[]){0}, &i), OK);
  CHECK(i, 1);
  CHECK(sl_flat_map_upper_bound(map, (int[]){-1}, &i), OK);
  CHECK(i, 0);
  CHECK(sl_flat_map_upper_bound(map, (int[]){5}, &i), OK);
  CHECK(i, 4);
  CHECK(sl_flat_map_upper_bound(map, (int[]){24}, &i), OK);
  CHECK(i, 5);
  CHECK(sl_flat_map_upper_bound(map, (int[]){25}, &i), OK);
  CHECK(i, 6);
  CHECK(sl_flat_map_upper_bound(map, (int[]){26}, &i), OK);
  CHECK(i, 6);

  CHECK(sl_free_flat_map(NULL), BAD_ARG);
  CHECK(sl_free_flat_map(map), OK);
  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}
