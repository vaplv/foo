#include "stdlib/sl_hash_table.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "utest/utest.h"

struct pair{
  int key;
  char val;
};

#define BAD_ARG SL_INVALID_ARGUMENT
#define BAD_AL SL_ALIGNMENT_ERROR
#define OK SL_NO_ERROR
#define SZK sizeof(int)
#define SZP sizeof(struct pair)
#define ALP ALIGNOF(struct pair)

static int
cmp(const void* p0, const void* p1)
{
  return ((struct pair*)p0)->key - ((struct pair*)p1)->key;
}

static const void*
key(const void*p)
{
  return (const void*)&(((struct pair*)p)->key);
}

int
main(int argc UNUSED, char** argv UNUSED)
{
  ALIGN(16) struct pair array[2] = {{0, 'a'}, {1, 'b'}};
  void* ptr;
  struct sl_hash_table* tbl = NULL;
  size_t count = 0;

  STATIC_ASSERT(!IS_ALIGNED(array + 1, 16), Unexpected_alignment);

  CHECK(sl_create_hash_table(0, 0, 0, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, 0, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, 0, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, 0, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZK, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, SZK, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, SZK, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, SZK, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, cmp, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, 0, cmp, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, 0, cmp, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, 0, cmp, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZK, cmp, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, SZK, cmp, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, SZK, cmp, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, SZK, cmp, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, NULL, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, 0, NULL, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, 0, NULL, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, 0, NULL, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZK, NULL, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, SZK, NULL, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, SZK, NULL, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, SZK, NULL, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, cmp, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, 0, cmp, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, 0, cmp, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, 0, cmp, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZK, cmp, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, SZK, cmp, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, SZK, cmp, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, SZK, cmp, key, NULL, NULL), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, 0, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, 0, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, 0, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZK, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, SZK, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, SZK, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, SZK, NULL, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, cmp, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, 0, cmp, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, 0, cmp, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, 0, cmp, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZK, cmp, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, SZK, cmp, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, SZK, cmp, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, SZK, cmp, NULL, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, NULL, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, 0, NULL, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, 0, NULL, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, 0, NULL, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZK, NULL, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, SZK, NULL, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, SZK, NULL, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, SZK, NULL, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, 0, cmp, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, 0, cmp, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, ALP, 0, cmp, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, 0, cmp, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(0, 0, SZK, cmp, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, 0, SZK, cmp, key, NULL, &tbl), BAD_AL);
  CHECK(sl_create_hash_table(0, ALP, SZK, cmp, key, NULL, &tbl), BAD_ARG);
  CHECK(sl_create_hash_table(SZP, ALP, SZK, cmp, key, NULL, &tbl), OK);

  CHECK(sl_hash_table_insert(NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_insert(tbl, NULL), BAD_ARG);
  CHECK(sl_hash_table_insert(NULL, (struct pair[]){{0, 'a'}}), BAD_ARG);
  CHECK(sl_hash_table_insert(tbl, (struct pair[]){{0, 'a'}}), OK);

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

  CHECK(sl_hash_table_insert(tbl, (struct pair[]){{0, 'a'}}), OK);
  CHECK(sl_hash_table_data_count(tbl, &count), OK);
  CHECK(count, 2);
  CHECK(sl_hash_table_used_bucket_count(tbl, &count), OK);
  CHECK(count, 1);

  CHECK(sl_hash_table_erase(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_erase(tbl, NULL, NULL), BAD_ARG);
  CHECK(sl_hash_table_erase(NULL, (int[]){1}, NULL), BAD_ARG);
  CHECK(sl_hash_table_erase(tbl, (int[]){1}, NULL), BAD_ARG);
  CHECK(sl_hash_table_erase(NULL, NULL, &count), BAD_ARG);
  CHECK(sl_hash_table_erase(tbl, NULL, &count), BAD_ARG);
  CHECK(sl_hash_table_erase(NULL, (int[]){1}, &count), BAD_ARG);
  CHECK(sl_hash_table_erase(tbl, (int[]){1}, &count), OK);
  CHECK(count, 0);
  CHECK(sl_hash_table_erase(tbl, (int[]){0}, &count), OK);
  CHECK(count, 2);
  CHECK(sl_hash_table_data_count(tbl, &count), OK);
  CHECK(count, 0);
  CHECK(sl_hash_table_used_bucket_count(tbl, &count), OK);
  CHECK(count, 0);

  CHECK(sl_hash_table_insert(tbl, (struct pair[]){{0, 'a'}}), OK);
  CHECK(sl_hash_table_insert(tbl, (struct pair[]){{1, 'b'}}), OK);
  CHECK(sl_hash_table_insert(tbl, (struct pair[]){{2, 'c'}}), OK);
  CHECK(sl_hash_table_insert(tbl, (struct pair[]){{3, 'd'}}), OK);
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
  CHECK(((struct pair*)ptr)->key, 0);
  CHECK(((struct pair*)ptr)->val, 'a');
  CHECK(sl_hash_table_find(tbl, (int[]){1}, &ptr), OK);
  NCHECK(ptr, NULL);
  CHECK(((struct pair*)ptr)->key, 1);
  CHECK(((struct pair*)ptr)->val, 'b');
  CHECK(sl_hash_table_find(tbl, (int[]){2}, &ptr), OK);
  NCHECK(ptr, NULL);
  CHECK(((struct pair*)ptr)->key, 2);
  CHECK(((struct pair*)ptr)->val, 'c');
  CHECK(sl_hash_table_find(tbl, (int[]){3}, &ptr), OK);
  NCHECK(ptr, NULL);
  CHECK(((struct pair*)ptr)->key, 3);
  CHECK(((struct pair*)ptr)->val, 'd');
  CHECK(sl_hash_table_find(tbl, (int[]){4}, &ptr), OK);
  CHECK(ptr, NULL);

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
        (SZP, 16, SZK, cmp, key, &mem_default_allocator, &tbl), OK);
  CHECK(sl_hash_table_find(tbl, (int[]){0}, &ptr), OK);
  CHECK(ptr, NULL);
  CHECK(sl_hash_table_insert(tbl, array + 0), OK);
  CHECK(sl_hash_table_insert(tbl, array + 1), BAD_AL);
  for(count = 0; count < 255; ++count) {
    ALIGN(16) const struct pair p = { count, count };
    CHECK(sl_hash_table_insert(tbl, &p), OK);
  }
  CHECK(sl_free_hash_table(tbl), OK);

  CHECK(sl_create_hash_table(SZP, ALP, SZK, cmp, key, NULL, &tbl), OK);
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
  CHECK(sl_free_hash_table(tbl), OK);
  CHECK(count, 16);

  return 0;
}

