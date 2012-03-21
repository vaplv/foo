#include "stdlib/sl_hash_table.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "utest/utest.h"
#include <stdbool.h>
#include <string.h>

#define BAD_ARG SL_INVALID_ARGUMENT
#define BAD_AL SL_ALIGNMENT_ERROR
#define OK SL_NO_ERROR
#define SZK sizeof(int)
#define ALK ALIGNOF(int)
#define SZD sizeof(char)
#define ALD ALIGNOF(char)

static bool
cmp(const void* p0, const void* p1)
{
  return *((const int*)p0) == *((const int*)p1);
}

static size_t
hash(const void*p)
{
  return sl_hash(p, sizeof(int));
}

int
main(int argc UNUSED, char** argv UNUSED)
{
  ALIGN(16) int array[2] = {0, 1};
  struct sl_pair pair;
  void* ptr;
  struct sl_hash_table* tbl = NULL;
  struct sl_hash_table_it it;
  size_t count = 0;
  bool bool_array[16];
  bool b = false;

  STATIC_ASSERT(!IS_ALIGNED(&array[1], 16), Unexpected_alignment);
  memset(&it, 0, sizeof(struct sl_hash_table_it));
  memset(&pair, 0, sizeof(struct sl_pair));

  CHECK(sl_create_hash_table(0, 0, 0, 0, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, 0, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, 0, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, 0, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, 0, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, 0, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, 0, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, 0, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, ALD, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, ALD, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, ALD, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, ALD, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, ALD, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, ALD, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, ALD, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, ALD, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, 0, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, 0, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, 0, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, 0, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, 0, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, 0, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, 0, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, 0, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, ALD, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, ALD, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, ALD, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, ALD, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, ALD, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, ALD, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, ALD, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, ALD, hash, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, 0, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, 0, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, 0, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, 0, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, 0, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, 0, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, 0, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, 0, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, ALD, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, ALD, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, ALD, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, ALD, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, ALD, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, ALD, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, ALD, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, ALD, NULL, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, 0, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, 0, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, 0, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, 0, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, 0, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, 0, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, 0, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, 0, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, ALD, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, ALD, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, ALD, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, ALD, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, ALD, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, ALD, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, ALD, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, ALD, hash, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, 0, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, 0, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, 0, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, 0, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, 0, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, 0, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, 0, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, 0, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, ALD, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, ALD, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, ALD, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, ALD, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, ALD, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, ALD, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, ALD, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, ALD, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, 0, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, 0, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, 0, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, 0, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, 0, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, 0, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, 0, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, 0, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, ALD, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, ALD, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, ALD, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, ALD, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, ALD, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, ALD, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, ALD, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, ALD, hash, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, 0, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, 0, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, 0, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, 0, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, 0, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, 0, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, 0, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, 0, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, ALD, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, ALD, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, ALD, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, ALD, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, ALD, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, ALD, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, SZD, ALD, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, ALD, NULL, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, 0, hash, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, 0, hash, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, 0, hash, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, 0, hash, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, 0, hash, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, 0, hash, cmp, NULL, &tbl), BAD_AL);
  CHECK(sl_create_hash_table(0, ALK, SZD, 0, hash, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, 0, hash, cmp, NULL, &tbl), BAD_AL);
  CHECK(sl_create_hash_table(0, 0, 0, ALD, hash, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, 0, ALD, hash, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALK, 0, ALD, hash, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, 0, ALD, hash, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZD, ALD, hash, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, 0, SZD, ALD, hash, cmp, NULL, &tbl), BAD_AL);
  CHECK(sl_create_hash_table(0, ALK, SZD, ALD, hash, cmp, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZK, ALK, SZD, ALD, hash, cmp, NULL, &tbl), OK);

  CHECK(sl_hash_table_insert(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_insert(tbl, NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_insert(NULL, (int[]){0}, NULL), BAD_ARG);
  CHECK(sl_hash_table_insert(tbl, (int[]){0}, NULL), BAD_ARG);
  CHECK(sl_hash_table_insert(NULL, NULL, (char[]){'a'}), BAD_ARG);
  CHECK(sl_hash_table_insert(tbl, NULL, (char[]){'a'}), BAD_ARG);
  CHECK(sl_hash_table_insert(NULL, (int[]){0}, (char[]){'a'}), BAD_ARG);
  CHECK(sl_hash_table_insert(tbl, (int[]){0}, (char[]){'a'}), OK);

  CHECK(sl_hash_table_data_count(NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_data_count(tbl, NULL), BAD_ARG);
  CHECK(sl_hash_table_data_count(NULL, &count), BAD_ARG);
  CHECK(sl_hash_table_data_count(tbl, &count), OK);
  CHECK(count, 1);

  CHECK(sl_hash_table_used_bucket_count(NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_used_bucket_count(tbl, NULL), BAD_ARG);
  CHECK(sl_hash_table_used_bucket_count(NULL, &count), BAD_ARG);
  CHECK(sl_hash_table_used_bucket_count(tbl, &count), OK);
  CHECK(count, 1);

  CHECK(sl_hash_table_insert(tbl, (int[]){0}, (char[]){'a'}), OK);
  CHECK(sl_hash_table_data_count(tbl, &count), OK);
  CHECK(count, 2);
  CHECK(sl_hash_table_used_bucket_count(tbl, &count), OK);
  CHECK(count, 1);

  CHECK(sl_hash_table_erase(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_erase(tbl, NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_erase(NULL, (int[]){1}, NULL), BAD_ARG);
  CHECK(sl_hash_table_erase(NULL, NULL, &count), BAD_ARG);
  CHECK(sl_hash_table_erase(tbl, NULL, &count), BAD_ARG);
  CHECK(sl_hash_table_erase(NULL, (int[]){1}, &count), BAD_ARG);
  CHECK(sl_hash_table_erase(tbl, (int[]){1}, &count), OK);
  CHECK(count, 0);
  CHECK(sl_hash_table_erase(tbl, (int[]){1}, NULL), OK);
  CHECK(sl_hash_table_erase(tbl, (int[]){0}, &count), OK);
  CHECK(count, 2);
  CHECK(sl_hash_table_data_count(tbl, &count), OK);
  CHECK(count, 0);
  CHECK(sl_hash_table_used_bucket_count(tbl, &count), OK);
  CHECK(count, 0);

  CHECK(sl_hash_table_insert(tbl, (int[]){0}, (char[]){'a'}), OK);
  CHECK(sl_hash_table_insert(tbl, (int[]){1}, (char[]){'b'}), OK);
  CHECK(sl_hash_table_insert(tbl, (int[]){2}, (char[]){'c'}), OK);
  CHECK(sl_hash_table_insert(tbl, (int[]){3}, (char[]){'d'}), OK);
  CHECK(sl_hash_table_data_count(tbl, &count), OK);
  CHECK(count, 4);
  CHECK(sl_hash_table_used_bucket_count(tbl, &count), OK);
  NCHECK(count, 0);

  CHECK(sl_hash_table_find(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_find(tbl, NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_find(NULL, (int[]){0}, NULL), BAD_ARG);
  CHECK(sl_hash_table_find(tbl, (int[]){0}, NULL), BAD_ARG);
  CHECK(sl_hash_table_find(NULL, NULL, &ptr), BAD_ARG);
  CHECK(sl_hash_table_find(tbl, NULL, &ptr), BAD_ARG);
  CHECK(sl_hash_table_find(NULL, (int[]){0}, &ptr), BAD_ARG);
  CHECK(sl_hash_table_find(tbl, (int[]){0}, &ptr), OK);
  NCHECK(ptr, NULL);
  CHECK(*(char*)ptr, 'a');
  CHECK(sl_hash_table_find(tbl, (int[]){1}, &ptr), OK);
  NCHECK(ptr, NULL);
  CHECK(*(char*)ptr, 'b');
  CHECK(sl_hash_table_find(tbl, (int[]){2}, &ptr), OK);
  NCHECK(ptr, NULL);
  CHECK(*(char*)ptr, 'c');
  CHECK(sl_hash_table_find(tbl, (int[]){3}, &ptr), OK);
  NCHECK(ptr, NULL);
  CHECK(*(char*)ptr, 'd');
  CHECK(sl_hash_table_find(tbl, (int[]){4}, &ptr), OK);
  CHECK(ptr, NULL);

  CHECK(sl_hash_table_find_pair(tbl, (int[]){0}, &pair), OK);
  CHECK(SL_IS_PAIR_VALID(&pair), true);
  CHECK(*(int*)pair.key, 0);
  CHECK(*(char*)pair.data, 'a');
  CHECK(sl_hash_table_find_pair(tbl, (int[]){1}, &pair), OK);
  CHECK(SL_IS_PAIR_VALID(&pair), true);
  CHECK(*(int*)pair.key, 1);
  CHECK(*(char*)pair.data, 'b');
  CHECK(sl_hash_table_find_pair(tbl, (int[]){2}, &pair), OK);
  CHECK(SL_IS_PAIR_VALID(&pair), true);
  CHECK(*(int*)pair.key, 2);
  CHECK(*(char*)pair.data, 'c');
  CHECK(sl_hash_table_find_pair(tbl, (int[]){3}, &pair), OK);
  CHECK(SL_IS_PAIR_VALID(&pair), true);
  CHECK(*(int*)pair.key, 3);
  CHECK(*(char*)pair.data, 'd');
  CHECK(sl_hash_table_find_pair(tbl, (int[]){4}, &pair), OK);
  CHECK(SL_IS_PAIR_VALID(&pair), false);

  CHECK(sl_hash_table_erase(tbl, (int[]){4}, &count), OK);
  CHECK(count, 0);
  CHECK(sl_hash_table_erase(tbl, (int[]){2}, &count), OK);
  CHECK(count, 1);
  CHECK(sl_hash_table_find(tbl, (int[]){2}, &ptr), OK);
  CHECK(ptr, NULL);
  CHECK(sl_hash_table_data_count(tbl, &count), OK);
  CHECK(count, 3);

  CHECK(sl_hash_table_clear(NULL), BAD_ARG);
  CHECK(sl_hash_table_clear(tbl), OK);
  CHECK(sl_hash_table_data_count(tbl, &count), OK);
  CHECK(count, 0);
  CHECK(sl_hash_table_used_bucket_count(tbl, &count), OK);
  CHECK(count, 0);

  CHECK(sl_free_hash_table(NULL), BAD_ARG);
  CHECK(sl_free_hash_table(tbl), OK);

  CHECK(sl_create_hash_table
        (SZK, 16, SZD, ALD, hash, cmp, &mem_default_allocator, &tbl), OK);
  CHECK(sl_hash_table_find(tbl, (int[]){0}, &ptr), OK);
  CHECK(ptr, NULL);
  CHECK(sl_hash_table_insert(tbl, array + 0, (char[]){'a'}), OK);
  CHECK(sl_hash_table_insert(tbl, array + 1, (char[]){'b'}), BAD_AL);
  for(count = 0; count < 255; ++count) {
    ALIGN(16) const int i = count;
    const char c = count;
    CHECK(sl_hash_table_insert(tbl, &i, &c), OK);
  }
  CHECK(sl_free_hash_table(tbl), OK);

  CHECK(sl_create_hash_table(SZK, ALK, SZD, ALD, hash, cmp, NULL, &tbl), OK);
  CHECK(sl_hash_table_resize(NULL, 0), BAD_ARG);
  CHECK(sl_hash_table_resize(tbl, 0), OK);
  CHECK(sl_hash_table_bucket_count(tbl, &count), OK);
  CHECK(count, 1);
  CHECK(sl_hash_table_resize(tbl, 6), OK);
  CHECK(sl_hash_table_bucket_count(tbl, &count), OK);
  CHECK(count, 8);
  CHECK(sl_hash_table_resize(tbl, 11), OK);
  CHECK(sl_hash_table_bucket_count(tbl, &count), OK);
  CHECK(count, 16);
  CHECK(sl_hash_table_resize(tbl, 6), OK);
  CHECK(sl_hash_table_bucket_count(tbl, &count), OK);
  CHECK(count, 16);
  CHECK(sl_hash_table_resize(tbl, 1), OK);
  CHECK(sl_hash_table_bucket_count(tbl, &count), OK);
  CHECK(count, 16);

  STATIC_ASSERT(sizeof(bool_array) / sizeof(bool) < 255, Unexpected_array_size);
  memset(bool_array, 0, sizeof(bool_array));
  for(count = 0; count < sizeof(bool_array) / sizeof(bool); ++count) {
    const char c = 'a' + (char)count;
    CHECK(sl_hash_table_insert(tbl, (int[]){count}, &c), OK);
  }

  CHECK(sl_hash_table_begin(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_begin(tbl, NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_begin(NULL, &it, NULL), BAD_ARG);
  CHECK(sl_hash_table_begin(tbl, &it, NULL), BAD_ARG);
  CHECK(sl_hash_table_begin(NULL, NULL, &b), BAD_ARG);
  CHECK(sl_hash_table_begin(tbl, NULL, &b), BAD_ARG);
  CHECK(sl_hash_table_begin(NULL, &it, &b), BAD_ARG);
  CHECK(sl_hash_table_begin(tbl, &it, &b), OK);
  CHECK(sl_hash_table_it_next(NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_it_next(&it, NULL), BAD_ARG);
  CHECK(sl_hash_table_it_next(NULL, &b), BAD_ARG);
  CHECK(b, false);
  do {
    const char c = *((char*)it.pair.data) - 'a';
    bool_array[(size_t)c] = true;
    CHECK(c,  *((int*)it.pair.key));
    CHECK(sl_hash_table_it_next(&it, &b), OK);
  } while(false == b);
  for(count = 0; count < sizeof(bool_array) / sizeof(bool); ++count)
    CHECK(bool_array[count], true);

  CHECK(sl_free_hash_table(tbl), OK);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}

