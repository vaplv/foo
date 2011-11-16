#include "stdlib/sl_sorted_vector.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "utest/utest.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

/* Shortened expressions. */
#define SZ sizeof
#define AL ALIGNOF
#define BAD_ARG SL_INVALID_ARGUMENT
#define BAD_ALIGN SL_ALIGNMENT_ERROR
#define OK SL_NO_ERROR

static int
cmp(const void* i0, const void* i1)
{
  int a, b;
  assert(i0 && i1);

  a = *(int*)i0;
  b = *(int*)i1;

  if(a < b)
    return -1;
  else if(a > b)
    return 1;
  else
    return 0;
}

int
main(int argc UNUSED, char** argv UNUSED)
{
  ALIGN(16) int array[4];
  struct sl_sorted_vector* vec = NULL;
  void* buffer = NULL;
  void* data = NULL;
  size_t capacity = 0;
  size_t capacity2 = 0;
  size_t id = 0;
  size_t len = 0;
  size_t sz = 0;
  size_t al = 0;

  CHECK(sl_create_sorted_vector(0, 0, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_sorted_vector(SZ(int), 0, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_sorted_vector(0, AL(int), NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_sorted_vector(SZ(int), AL(int), NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_create_sorted_vector(0, 0, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_sorted_vector(SZ(int), 0, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_sorted_vector(0, AL(int), cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_sorted_vector(SZ(int), AL(int), cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_sorted_vector(0, 0, NULL, NULL, &vec), BAD_ARG);
  CHECK(sl_create_sorted_vector(SZ(int), 0, NULL, NULL, &vec), BAD_ARG);
  CHECK(sl_create_sorted_vector(0, AL(int), NULL, NULL, &vec), BAD_ARG);
  CHECK(sl_create_sorted_vector(SZ(int), AL(int), NULL, NULL, &vec), BAD_ARG);
  CHECK(sl_create_sorted_vector(0, 0, cmp, NULL, NULL), BAD_ARG);
  CHECK(sl_create_sorted_vector(SZ(int), 0, cmp, NULL, &vec), BAD_ALIGN);
  CHECK(sl_create_sorted_vector(0, AL(int), cmp, NULL, &vec), BAD_ARG);
  CHECK(sl_create_sorted_vector(SZ(int), AL(int), cmp, NULL, &vec), OK);

  CHECK(sl_sorted_vector_insert(NULL, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_insert(vec, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_insert(NULL, (int[]){9}), BAD_ARG);
  CHECK(sl_sorted_vector_insert(vec, (int[]){9}), OK);

  CHECK(sl_sorted_vector_buffer(NULL, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_buffer(vec, NULL, NULL, NULL, NULL), OK);
  CHECK(sl_sorted_vector_buffer(NULL, &len, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_buffer(vec, &len, NULL, NULL, NULL), OK);
  CHECK(len, 1);
  CHECK(sl_sorted_vector_buffer(NULL, NULL, &sz, NULL, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_buffer(vec, &len, &sz, NULL, NULL), OK);
  CHECK(len, 1);
  CHECK(sz, SZ(int));
  CHECK(sl_sorted_vector_buffer(vec, NULL, &sz, NULL, NULL), OK);
  CHECK(sz, SZ(int));
  CHECK(sl_sorted_vector_buffer(NULL, NULL, NULL, &al, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_buffer(vec, NULL, NULL, &al, NULL), OK);
  CHECK(al, AL(int));
  CHECK(sl_sorted_vector_buffer(vec, &len, NULL, &al, NULL), OK);
  CHECK(len, 1);
  CHECK(al, AL(int));
  CHECK(sl_sorted_vector_buffer(vec, NULL, &sz, &al, NULL), OK);
  CHECK(sz, SZ(int));
  CHECK(al, AL(int));
  CHECK(sl_sorted_vector_buffer(vec, &len, &sz, &al, NULL), OK);
  CHECK(len, 1);
  CHECK(sz, SZ(int));
  CHECK(al, AL(int));
  CHECK(sl_sorted_vector_buffer(vec, &len, NULL, NULL, &buffer), OK);
  CHECK(len, 1);
  CHECK(((int*)buffer)[0], 9);

  CHECK(sl_sorted_vector_insert(vec, (int[]){9}), BAD_ARG);
  CHECK(sl_sorted_vector_insert(vec, (int[]){0}), OK);
  CHECK(sl_sorted_vector_insert(vec, (int[]){-1}), OK);
  CHECK(sl_sorted_vector_insert(vec, (int[]){5}), OK);
  CHECK(sl_sorted_vector_insert(vec, (int[]){3}), OK);
  CHECK(sl_sorted_vector_insert(vec, (int[]){2}), OK);
  CHECK(sl_sorted_vector_insert(vec, (int[]){0}), BAD_ARG);
  CHECK(sl_sorted_vector_insert(vec, (int[]){7}), OK);
  CHECK(sl_sorted_vector_insert(vec, (int[]){5}), BAD_ARG);
  CHECK(sl_sorted_vector_insert(vec, (int[]){1}), OK);
  CHECK(sl_sorted_vector_insert(vec, (int[]){8}), OK);
  CHECK(sl_sorted_vector_insert(vec, (int[]){6}), OK);
  CHECK(sl_sorted_vector_insert(vec, (int[]){8}), BAD_ARG);
  CHECK(sl_sorted_vector_insert(vec, (int[]){-1}), BAD_ARG);
  CHECK(sl_sorted_vector_insert(vec, (int[]){4}), OK);
  CHECK(sl_sorted_vector_buffer(vec, &len, NULL, NULL, &buffer), OK);
  CHECK(len, 11);
  CHECK(((int*)buffer)[0], -1);
  CHECK(((int*)buffer)[1], 0);
  CHECK(((int*)buffer)[2], 1);
  CHECK(((int*)buffer)[3], 2);
  CHECK(((int*)buffer)[4], 3);
  CHECK(((int*)buffer)[5], 4);
  CHECK(((int*)buffer)[6], 5);
  CHECK(((int*)buffer)[7], 6);
  CHECK(((int*)buffer)[8], 7);
  CHECK(((int*)buffer)[9], 8);
  CHECK(((int*)buffer)[10], 9);

  CHECK(sl_sorted_vector_length(NULL, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_length(vec, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_length(NULL, &len), BAD_ARG);
  CHECK(sl_sorted_vector_length(vec, &len), OK);
  CHECK(len, 11);
  CHECK(sl_sorted_vector_insert(vec, (int[]){-5}), OK);
  CHECK(sl_sorted_vector_length(vec, &len), OK);
  CHECK(len, 12);
  CHECK(sl_sorted_vector_buffer(vec, &len, NULL, NULL, &buffer), OK);
  CHECK(len, 12);
  CHECK(((int*)buffer)[0], -5);
  CHECK(((int*)buffer)[1], -1);
  CHECK(((int*)buffer)[2], 0);
  CHECK(((int*)buffer)[3], 1);
  CHECK(((int*)buffer)[4], 2);
  CHECK(((int*)buffer)[5], 3);
  CHECK(((int*)buffer)[6], 4);
  CHECK(((int*)buffer)[7], 5);
  CHECK(((int*)buffer)[8], 6);
  CHECK(((int*)buffer)[9], 7);
  CHECK(((int*)buffer)[10], 8);
  CHECK(((int*)buffer)[11], 9);

  CHECK(sl_sorted_vector_find(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_find(vec, NULL, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_find(NULL, (int[]){-2}, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_find(vec, (int[]){-2}, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_find(NULL, NULL, &id), BAD_ARG);
  CHECK(sl_sorted_vector_find(vec, NULL, &id), BAD_ARG);
  CHECK(sl_sorted_vector_find(NULL, (int[]){-2}, &id), BAD_ARG);
  CHECK(sl_sorted_vector_find(vec, (int[]){-2}, &id), OK);
  CHECK(id, len);
  CHECK(sl_sorted_vector_find(vec, (int[]){-1}, &id), OK);
  CHECK(id, 1);
  CHECK(sl_sorted_vector_find(vec, (int[]){7}, &id), OK);
  CHECK(id, 9);
  CHECK(sl_sorted_vector_find(vec, (int[]){-5}, &id), OK);
  CHECK(id, 0);
  CHECK(sl_sorted_vector_find(vec, (int[]){9}, &id), OK);
  CHECK(id, 11);
  CHECK(sl_sorted_vector_find(vec, (int[]){0}, &id), OK);
  CHECK(id, 2);
  CHECK(sl_sorted_vector_find(vec, (int[]){33}, &id), OK);
  CHECK(id, len);

  CHECK(sl_sorted_vector_at(NULL, SIZE_MAX, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_at(vec, SIZE_MAX, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_at(NULL, 0, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_at(vec, 0, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_at(NULL, SIZE_MAX, &data), BAD_ARG);
  CHECK(sl_sorted_vector_at(vec, SIZE_MAX, &data), BAD_ARG);
  CHECK(sl_sorted_vector_at(NULL, 0, &data), BAD_ARG);

  for(id = 0; id < len; ++id) {
    CHECK(sl_sorted_vector_at(vec, id, &data), OK);
    CHECK(((int*)buffer)[id], *(int*)data);
  }

  CHECK(sl_sorted_vector_remove(NULL, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_remove(vec, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_remove(NULL, (int[]){-3}), BAD_ARG);
  CHECK(sl_sorted_vector_remove(vec, (int[]){-3}), BAD_ARG);
  CHECK(sl_sorted_vector_remove(vec, (int[]){7}), OK);

  CHECK(sl_sorted_vector_buffer(vec, &len, NULL, NULL, &buffer), OK);
  CHECK(len, 11);
  CHECK(((int*)buffer)[0], -5);
  CHECK(((int*)buffer)[1], -1);
  CHECK(((int*)buffer)[2], 0);
  CHECK(((int*)buffer)[3], 1);
  CHECK(((int*)buffer)[4], 2);
  CHECK(((int*)buffer)[5], 3);
  CHECK(((int*)buffer)[6], 4);
  CHECK(((int*)buffer)[7], 5);
  CHECK(((int*)buffer)[8], 6);
  CHECK(((int*)buffer)[9], 8);
  CHECK(((int*)buffer)[10], 9);

  CHECK(sl_sorted_vector_remove(vec, (int[]){-5}), OK);
  CHECK(sl_sorted_vector_remove(vec, (int[]){-1}), OK);
  CHECK(sl_sorted_vector_remove(vec, (int[]){9}), OK);
  CHECK(sl_sorted_vector_remove(vec, (int[]){3}), OK);
  CHECK(sl_sorted_vector_remove(vec, (int[]){3}), BAD_ARG);
  CHECK(sl_sorted_vector_remove(vec, (int[]){6}), OK);
  CHECK(sl_sorted_vector_buffer(vec, &len, NULL, NULL, &buffer), OK);
  CHECK(len, 6);
  CHECK(((int*)buffer)[0], 0);
  CHECK(((int*)buffer)[1], 1);
  CHECK(((int*)buffer)[2], 2);
  CHECK(((int*)buffer)[3], 4);
  CHECK(((int*)buffer)[4], 5);
  CHECK(((int*)buffer)[5], 8);

  CHECK(sl_sorted_vector_capacity(NULL, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_capacity(vec, NULL), BAD_ARG);
  CHECK(sl_sorted_vector_capacity(NULL, &capacity), BAD_ARG);
  CHECK(sl_sorted_vector_capacity(vec, &capacity), OK);
  CHECK(capacity >= len, true);

  CHECK(sl_sorted_vector_reserve(NULL, 0), BAD_ARG);
  CHECK(sl_sorted_vector_reserve(vec, 0), OK);
  CHECK(sl_sorted_vector_capacity(vec, &capacity), OK);
  NCHECK(capacity, 0);
  CHECK(sl_sorted_vector_reserve(vec, 2 * capacity), OK);
  CHECK(sl_sorted_vector_capacity(vec, &capacity2), OK);
  CHECK(capacity2, 2 * capacity);
  CHECK(sl_sorted_vector_buffer(vec, &len, NULL, NULL, &buffer), OK);
  CHECK(len, 6);
  CHECK(((int*)buffer)[0], 0);
  CHECK(((int*)buffer)[1], 1);
  CHECK(((int*)buffer)[2], 2);
  CHECK(((int*)buffer)[3], 4);
  CHECK(((int*)buffer)[4], 5);
  CHECK(((int*)buffer)[5], 8);

  CHECK(sl_sorted_vector_capacity(vec, &capacity), OK);
  for(id = 9; len != capacity; ++id) {
    CHECK(sl_sorted_vector_insert(vec, &id), OK);
    CHECK(sl_sorted_vector_length(vec, &len), OK);
  }

  CHECK(sl_sorted_vector_capacity(vec, &capacity2), OK);
  CHECK(capacity2, capacity);
  CHECK(sl_sorted_vector_insert(vec, &id), OK);
  CHECK(sl_sorted_vector_capacity(vec, &capacity2), OK);
  CHECK(capacity2, 2 * capacity);

  CHECK(sl_sorted_vector_buffer(vec, &len, NULL, NULL, &buffer), OK);
  for(id = 1; id < len; ++id) {
    CHECK(((int*)buffer)[id - 1] < ((int*)buffer)[id], true);
  }

  CHECK(sl_sorted_vector_capacity(vec, &capacity), OK);
  CHECK(sl_clear_sorted_vector(NULL), BAD_ARG);
  CHECK(sl_clear_sorted_vector(vec), OK);
  CHECK(sl_sorted_vector_length(vec, &len), OK);
  CHECK(len, 0);
  CHECK(sl_sorted_vector_buffer(vec, &len, &sz, &al, &buffer), OK);
  CHECK(len, 0);
  CHECK(sz, SZ(int));
  CHECK(al, AL(int));
  CHECK(sl_sorted_vector_capacity(vec, &capacity2), OK);
  CHECK(capacity, capacity2);

  CHECK(sl_free_sorted_vector(NULL), BAD_ARG);
  CHECK(sl_free_sorted_vector(vec), OK);

  CHECK(sl_create_sorted_vector
        (SZ(int), 16, cmp, &mem_default_allocator, &vec), OK);
  array[0] = 0;
  array[1] = 1;
  array[2] = 2;
  array[3] = 3;
  CHECK(sl_sorted_vector_insert(vec, &array[0]), OK);
  CHECK(sl_sorted_vector_insert(vec, &array[1]), BAD_ALIGN);
  CHECK(sl_free_sorted_vector(vec), OK);

  return 0;
}

