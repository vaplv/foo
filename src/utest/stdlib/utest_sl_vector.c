#include "stdlib/sl_vector.h"
#include "sys/sys.h"
#include "utest/utest.h"
#include <stdbool.h>
#include <stdlib.h>

#define BAD_ARG SL_INVALID_ARGUMENT
#define OK SL_NO_ERROR

int
main(int argc UNUSED, char** argv UNUSED)
{
  struct sl_vector* vec = NULL;
  void* data = NULL;
  size_t len = 0;
  size_t size = 0;
  size_t alignment = 0;
  ALIGN(16) int i[4];


  CHECK(sl_create_vector(0, 0, NULL), BAD_ARG);
  CHECK(sl_create_vector(0, 0, &vec), BAD_ARG);
  CHECK(sl_create_vector(sizeof(int), 0, NULL), BAD_ARG);
  CHECK(sl_create_vector(sizeof(int), 3, &vec), SL_ALIGNMENT_ERROR);
  CHECK(sl_create_vector(sizeof(int), ALIGNOF(int), &vec), OK);

  CHECK(sl_vector_length(NULL, NULL), BAD_ARG);
  CHECK(sl_vector_length(vec, NULL), BAD_ARG);
  CHECK(sl_vector_length(NULL, &len), BAD_ARG);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 0);

  CHECK(sl_vector_push_back(NULL, NULL), BAD_ARG);
  CHECK(sl_vector_push_back(vec, NULL), BAD_ARG);
  CHECK(sl_vector_push_back(NULL, (int[]){0}), BAD_ARG);
  CHECK(sl_vector_push_back(vec, (int[]){0}), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 1);

  CHECK(sl_vector_push_back(vec, (int[]){1}), OK);
  CHECK(sl_vector_push_back(vec, (int[]){2}), OK);
  CHECK(sl_vector_push_back(vec, (int[]){3}), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 4);

  CHECK(sl_vector_pop_back(NULL), BAD_ARG);
  CHECK(sl_vector_pop_back(vec), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 3);

  CHECK(sl_vector_pop_back(vec), OK);
  CHECK(sl_vector_pop_back(vec), OK);
  CHECK(sl_vector_pop_back(vec), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 0);

  CHECK(sl_vector_pop_back(vec), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 0);

  CHECK(sl_vector_push_back(vec, (int[]){0}), OK);
  CHECK(sl_vector_push_back(vec, (int[]){1}), OK);
  CHECK(sl_vector_push_back(vec, (int[]){2}), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 3);

  CHECK(sl_clear_vector(NULL), BAD_ARG);
  CHECK(sl_clear_vector(vec), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 0);

  CHECK(sl_vector_push_back(vec, (int[]){0}), OK);
  CHECK(sl_vector_push_back(vec, (int[]){1}), OK);
  CHECK(sl_vector_push_back(vec, (int[]){2}), OK);
  CHECK(sl_vector_push_back(vec, (int[]){3}), OK);
  CHECK(sl_vector_push_back(vec, (int[]){4}), OK);
  CHECK(sl_vector_push_back(vec, (int[]){5}), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 6);

  CHECK(sl_vector_at(NULL, 0, NULL), BAD_ARG);
  CHECK(sl_vector_at(vec, 0, NULL), BAD_ARG);
  CHECK(sl_vector_at(NULL, 0, &data), BAD_ARG);
  CHECK(sl_vector_at(vec, 0, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(vec, 2, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(vec, 5, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(vec, 6, &data), BAD_ARG);

  CHECK(sl_vector_insert(NULL, 6, NULL), BAD_ARG);
  CHECK(sl_vector_insert(vec, 6, NULL), BAD_ARG);
  CHECK(sl_vector_insert(NULL, 2, NULL), BAD_ARG);
  CHECK(sl_vector_insert(vec, 2, NULL), BAD_ARG);
  CHECK(sl_vector_insert(NULL, 7, (int[]){6}), BAD_ARG);
  CHECK(sl_vector_insert(vec, 7, (int[]){6}), BAD_ARG);
  CHECK(sl_vector_insert(NULL, 2, (int[]){6}), BAD_ARG);
  CHECK(sl_vector_insert(vec, 2, (int[]){6}), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 7);
  CHECK(sl_vector_at(vec, 0, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(vec, 1, &data), OK);
  CHECK(*(int*)data, 1);
  CHECK(sl_vector_at(vec, 2, &data), OK);
  CHECK(*(int*)data, 6);
  CHECK(sl_vector_at(vec, 3, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(vec, 4, &data), OK);
  CHECK(*(int*)data, 3);
  CHECK(sl_vector_at(vec, 5, &data), OK);
  CHECK(*(int*)data, 4);
  CHECK(sl_vector_at(vec, 6, &data), OK);
  CHECK(*(int*)data, 5);

  CHECK(sl_vector_insert(vec, 6, (int[]){7}), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 8);
  CHECK(sl_vector_at(vec, 0, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(vec, 1, &data), OK);
  CHECK(*(int*)data, 1);
  CHECK(sl_vector_at(vec, 2, &data), OK);
  CHECK(*(int*)data, 6);
  CHECK(sl_vector_at(vec, 3, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(vec, 4, &data), OK);
  CHECK(*(int*)data, 3);
  CHECK(sl_vector_at(vec, 5, &data), OK);
  CHECK(*(int*)data, 4);
  CHECK(sl_vector_at(vec, 6, &data), OK);
  CHECK(*(int*)data, 7);
  CHECK(sl_vector_at(vec, 7, &data), OK);
  CHECK(*(int*)data, 5);

  CHECK(sl_vector_insert(vec, 5, data), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 9);
  CHECK(sl_vector_at(vec, 0, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(vec, 1, &data), OK);
  CHECK(*(int*)data, 1);
  CHECK(sl_vector_at(vec, 2, &data), OK);
  CHECK(*(int*)data, 6);
  CHECK(sl_vector_at(vec, 3, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(vec, 4, &data), OK);
  CHECK(*(int*)data, 3);
  CHECK(sl_vector_at(vec, 5, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(vec, 6, &data), OK);
  CHECK(*(int*)data, 4);
  CHECK(sl_vector_at(vec, 7, &data), OK);
  CHECK(*(int*)data, 7);
  CHECK(sl_vector_at(vec, 8, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(vec, 9, &data), BAD_ARG);

  CHECK(sl_vector_push_back(vec, (int[]){9}), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 10);
  CHECK(sl_vector_at(vec, 9, &data), OK);
  CHECK(*(int*)data, 9);
  CHECK(sl_vector_insert(vec, 0, data), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 11);

  CHECK(sl_vector_at(vec, 0, &data), OK);
  CHECK(*(int*)data, 9);
  CHECK(sl_vector_at(vec, 1, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(vec, 2, &data), OK);
  CHECK(*(int*)data, 1);
  CHECK(sl_vector_at(vec, 3, &data), OK);
  CHECK(*(int*)data, 6);
  CHECK(sl_vector_at(vec, 4, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(vec, 5, &data), OK);
  CHECK(*(int*)data, 3);
  CHECK(sl_vector_at(vec, 6, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(vec, 7, &data), OK);
  CHECK(*(int*)data, 4);
  CHECK(sl_vector_at(vec, 8, &data), OK);
  CHECK(*(int*)data, 7);
  CHECK(sl_vector_at(vec, 9, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(vec, 10, &data), OK);
  CHECK(*(int*)data, 9);

  CHECK(sl_vector_remove(NULL, 12), BAD_ARG);
  CHECK(sl_vector_remove(vec, 12), BAD_ARG);
  CHECK(sl_vector_remove(NULL, 5), BAD_ARG);
  CHECK(sl_vector_remove(vec, 5), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 10);
  CHECK(sl_vector_at(vec, 0, &data), OK);
  CHECK(*(int*)data, 9);
  CHECK(sl_vector_at(vec, 1, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(vec, 2, &data), OK);
  CHECK(*(int*)data, 1);
  CHECK(sl_vector_at(vec, 3, &data), OK);
  CHECK(*(int*)data, 6);
  CHECK(sl_vector_at(vec, 4, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(vec, 5, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(vec, 6, &data), OK);
  CHECK(*(int*)data, 4);
  CHECK(sl_vector_at(vec, 7, &data), OK);
  CHECK(*(int*)data, 7);
  CHECK(sl_vector_at(vec, 8, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(vec, 9, &data), OK);
  CHECK(*(int*)data, 9);

  CHECK(sl_vector_remove(vec, 9), OK);
  CHECK(sl_vector_remove(vec, 0), OK);
  CHECK(sl_vector_remove(vec, 2), OK);
  CHECK(sl_vector_remove(vec, 5), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 6);
  CHECK(sl_vector_at(vec, 0, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(vec, 1, &data), OK);
  CHECK(*(int*)data, 1);
  CHECK(sl_vector_at(vec, 2, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(vec, 3, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(vec, 4, &data), OK);
  CHECK(*(int*)data, 4);
  CHECK(sl_vector_at(vec, 5, &data), OK);
  CHECK(*(int*)data, 5);

  CHECK(sl_vector_buffer(NULL, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_vector_buffer(vec, NULL, NULL, NULL, NULL), OK);
  CHECK(sl_vector_buffer(vec, &len, NULL, NULL, NULL), OK);
  CHECK(len, 6);
  CHECK(sl_vector_buffer(vec, NULL, &size, NULL, NULL), OK);
  CHECK(size, sizeof(int));
  CHECK(sl_vector_buffer(vec, NULL, NULL, &alignment, NULL), OK);
  CHECK(alignment, ALIGNOF(int));
  CHECK(sl_vector_buffer(vec, &len, &size, &alignment, &data), OK);
  CHECK(len, 6);
  CHECK(size, sizeof(int));
  CHECK(alignment, ALIGNOF(int));
  CHECK(((int*)data)[0], 0);
  CHECK(((int*)data)[1], 1);
  CHECK(((int*)data)[2], 2);
  CHECK(((int*)data)[3], 5);
  CHECK(((int*)data)[4], 4);
  CHECK(((int*)data)[5], 5);

  CHECK(sl_vector_resize(NULL, 9, NULL), BAD_ARG);
  CHECK(sl_vector_resize(NULL, 9, (int[]){-1}), BAD_ARG);
  CHECK(sl_vector_resize(vec, 9, (int[]){-1}), OK);
  CHECK(sl_vector_buffer(vec, &len, NULL, NULL, &data), OK);
  CHECK(len, 9);
  CHECK(((int*)data)[0], 0);
  CHECK(((int*)data)[1], 1);
  CHECK(((int*)data)[2], 2);
  CHECK(((int*)data)[3], 5);
  CHECK(((int*)data)[4], 4);
  CHECK(((int*)data)[5], 5);
  CHECK(((int*)data)[6], -1);
  CHECK(((int*)data)[7], -1);
  CHECK(((int*)data)[8], -1);

  CHECK(sl_vector_resize(vec, 17, (int[]){-2}), OK);
  CHECK(sl_vector_buffer(vec, &len, NULL, NULL, &data), OK);
  CHECK(len, 17);
  CHECK(((int*)data)[7], -1);
  CHECK(((int*)data)[8], -1);
  CHECK(((int*)data)[9], -2);
  CHECK(((int*)data)[10], -2);
  CHECK(((int*)data)[11], -2);
  CHECK(((int*)data)[12], -2);
  CHECK(((int*)data)[13], -2);
  CHECK(((int*)data)[14], -2);
  CHECK(((int*)data)[15], -2);
  CHECK(((int*)data)[16], -2);

  CHECK(sl_vector_resize(vec, 20, NULL), OK);
  CHECK(sl_vector_buffer(vec, &len, NULL, NULL, &data), OK);
  CHECK(len, 20);
  CHECK(((int*)data)[15], -2);
  CHECK(((int*)data)[16], -2);
  CHECK(((int*)data)[17], 0);
  CHECK(((int*)data)[18], 0);
  CHECK(((int*)data)[19], 0);

  CHECK(sl_vector_resize(vec, 7, NULL), OK);
  CHECK(sl_vector_buffer(vec, &len, NULL, NULL, &data), OK);
  CHECK(len, 7);
  CHECK(((int*)data)[0], 0);
  CHECK(((int*)data)[1], 1);
  CHECK(((int*)data)[2], 2);
  CHECK(((int*)data)[3], 5);
  CHECK(((int*)data)[4], 4);
  CHECK(((int*)data)[5], 5);
  CHECK(((int*)data)[6], -1);
 
  CHECK(sl_clear_vector(vec), OK);
  CHECK(sl_vector_insert(vec, 1, (int[]){1}), BAD_ARG);
  CHECK(sl_vector_buffer(vec, &len, NULL, NULL, &data), OK);
  CHECK(len, 0);
  CHECK(data, NULL);

  CHECK(sl_vector_insert(vec, 0, (int[]){1}), OK);
  CHECK(sl_vector_insert(vec, 1, (int[]){2}), OK);
  CHECK(sl_vector_insert(vec, 2, (int[]){4}), OK);
  CHECK(sl_vector_insert(vec, 0, (int[]){0}), OK);
  CHECK(sl_vector_insert(vec, 3, (int[]){3}), OK);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 5);
  CHECK(sl_vector_buffer(vec, &len, NULL, NULL, &data), OK);
  CHECK(len, 5);
  CHECK(((int*)data)[0], 0);
  CHECK(((int*)data)[1], 1);
  CHECK(((int*)data)[2], 2);
  CHECK(((int*)data)[3], 3);
  CHECK(((int*)data)[4], 4);

  CHECK(sl_free_vector(NULL), BAD_ARG);
  CHECK(sl_free_vector(vec), OK);

  CHECK(sl_create_vector(sizeof(int), 16, &vec), OK);

  CHECK(sl_vector_capacity(NULL, NULL), BAD_ARG);
  CHECK(sl_vector_capacity(vec, NULL), BAD_ARG);
  CHECK(sl_vector_capacity(NULL, &len), BAD_ARG);
  CHECK(sl_vector_capacity(vec, &len), OK);
  CHECK(len, 0);

  CHECK(sl_vector_reserve(NULL, 0), BAD_ARG);
  CHECK(sl_vector_reserve(vec, 0), OK);
  CHECK(sl_vector_capacity(vec, &len), OK);
  CHECK(len, 0);
  CHECK(sl_vector_reserve(vec, 4), OK);
  CHECK(sl_vector_capacity(vec, &len), OK);
  CHECK(len, 4);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 0);
  CHECK(sl_vector_reserve(vec, 2), OK);
  CHECK(sl_vector_capacity(vec, &len), OK);
  CHECK(len, 4);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 0);
  CHECK(sl_vector_push_back(vec, (int[]){0}), OK);
  CHECK(sl_vector_push_back(vec, (int[]){1}), OK);
  CHECK(sl_vector_push_back(vec, (int[]){2}), OK);
  CHECK(sl_vector_push_back(vec, (int[]){3}), OK);
  CHECK(sl_vector_capacity(vec, &len), OK);
  CHECK(len, 4);
  CHECK(sl_vector_length(vec, &len), OK);
  CHECK(len, 4);
  CHECK(sl_vector_push_back(vec, (int[]){4}), OK);
  CHECK(sl_vector_capacity(vec, &len), OK);
  NCHECK(len, 4);

  CHECK(sl_vector_push_back(vec, i + 1), SL_ALIGNMENT_ERROR);
  CHECK(sl_vector_push_back(vec, (int[]){1}), OK);
  CHECK(sl_vector_insert(vec, 0, i + 1), SL_ALIGNMENT_ERROR);
  CHECK(sl_free_vector(vec), OK);

  return 0;
}
