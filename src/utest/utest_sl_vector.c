#include "stdlib/sl_vector.h"
#include "stdlib/sl_context.h"
#include "sys/sys.h"
#include "utest/utest.h"
#include <stdbool.h>
#include <stdlib.h>

#define BAD_ARG SL_INVALID_ARGUMENT
#define OK SL_NO_ERROR

int
main(int argc UNUSED, char** argv UNUSED)
{
  struct sl_context* ctxt = NULL;
  struct sl_vector* vec = NULL;
  void* data = NULL;
  size_t len = 0;
  size_t size = 0;
  size_t alignment = 0;
  ALIGN(16) int i[4];

  CHECK(sl_create_context(&ctxt), OK);

  CHECK(sl_create_vector(NULL, 0, 0, NULL), BAD_ARG);
  CHECK(sl_create_vector(ctxt, 0, 0, NULL), BAD_ARG);
  CHECK(sl_create_vector(NULL, 0, 0, &vec), BAD_ARG);
  CHECK(sl_create_vector(ctxt, 0, 0, &vec), BAD_ARG);
  CHECK(sl_create_vector(NULL, sizeof(int), 0, NULL), BAD_ARG);
  CHECK(sl_create_vector(ctxt, sizeof(int), 0, NULL), BAD_ARG);
  CHECK(sl_create_vector(NULL, sizeof(int), 3, &vec), BAD_ARG);
  CHECK(sl_create_vector(ctxt, sizeof(int), 3, &vec), SL_ALIGNMENT_ERROR);
  CHECK(sl_create_vector(ctxt, sizeof(int), ALIGNOF(int), &vec), OK);

  CHECK(sl_vector_length(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_vector_length(ctxt, NULL, NULL), BAD_ARG);
  CHECK(sl_vector_length(NULL, vec, NULL), BAD_ARG);
  CHECK(sl_vector_length(ctxt, vec, NULL), BAD_ARG);
  CHECK(sl_vector_length(NULL, NULL, &len), BAD_ARG);
  CHECK(sl_vector_length(ctxt, NULL, &len), BAD_ARG);
  CHECK(sl_vector_length(NULL, vec, &len), BAD_ARG);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 0);

  CHECK(sl_vector_push_back(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_vector_push_back(ctxt, NULL, NULL), BAD_ARG);
  CHECK(sl_vector_push_back(NULL, vec, NULL), BAD_ARG);
  CHECK(sl_vector_push_back(ctxt, vec, NULL), BAD_ARG);
  CHECK(sl_vector_push_back(NULL, NULL, (int[]){0}), BAD_ARG);
  CHECK(sl_vector_push_back(ctxt, NULL, (int[]){0}), BAD_ARG);
  CHECK(sl_vector_push_back(NULL, vec, (int[]){0}), BAD_ARG);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){0}), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 1);

  CHECK(sl_vector_push_back(ctxt, vec, (int[]){1}), OK);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){2}), OK);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){3}), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 4);

  CHECK(sl_vector_pop_back(NULL, NULL), BAD_ARG);
  CHECK(sl_vector_pop_back(ctxt, NULL), BAD_ARG);
  CHECK(sl_vector_pop_back(NULL, vec), BAD_ARG);
  CHECK(sl_vector_pop_back(ctxt, vec), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 3);

  CHECK(sl_vector_pop_back(ctxt, vec), OK);
  CHECK(sl_vector_pop_back(ctxt, vec), OK);
  CHECK(sl_vector_pop_back(ctxt, vec), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 0);

  CHECK(sl_vector_pop_back(ctxt, vec), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 0);

  CHECK(sl_vector_push_back(ctxt, vec, (int[]){0}), OK);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){1}), OK);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){2}), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 3);

  CHECK(sl_clear_vector(NULL, NULL), BAD_ARG);
  CHECK(sl_clear_vector(ctxt, NULL), BAD_ARG);
  CHECK(sl_clear_vector(NULL, vec), BAD_ARG);
  CHECK(sl_clear_vector(ctxt, vec), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 0);

  CHECK(sl_vector_push_back(ctxt, vec, (int[]){0}), OK);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){1}), OK);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){2}), OK);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){3}), OK);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){4}), OK);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){5}), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 6);

  CHECK(sl_vector_at(NULL, NULL, 0, NULL), BAD_ARG);
  CHECK(sl_vector_at(ctxt, NULL, 0, NULL), BAD_ARG);
  CHECK(sl_vector_at(NULL, vec, 0, NULL), BAD_ARG);
  CHECK(sl_vector_at(ctxt, vec, 0, NULL), BAD_ARG);
  CHECK(sl_vector_at(NULL, NULL, 0, &data), BAD_ARG);
  CHECK(sl_vector_at(ctxt, NULL, 0, &data), BAD_ARG);
  CHECK(sl_vector_at(NULL, vec, 0, &data), BAD_ARG);
  CHECK(sl_vector_at(ctxt, vec, 0, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(ctxt, vec, 2, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(ctxt, vec, 5, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(ctxt, vec, 6, &data), BAD_ARG);

  CHECK(sl_vector_insert(NULL, NULL, 6, NULL), BAD_ARG);
  CHECK(sl_vector_insert(ctxt, NULL, 6, NULL), BAD_ARG);
  CHECK(sl_vector_insert(NULL, vec, 6, NULL), BAD_ARG);
  CHECK(sl_vector_insert(ctxt, vec, 6, NULL), BAD_ARG);
  CHECK(sl_vector_insert(NULL, NULL, 2, NULL), BAD_ARG);
  CHECK(sl_vector_insert(ctxt, NULL, 2, NULL), BAD_ARG);
  CHECK(sl_vector_insert(NULL, vec, 2, NULL), BAD_ARG);
  CHECK(sl_vector_insert(ctxt, vec, 2, NULL), BAD_ARG);
  CHECK(sl_vector_insert(NULL, NULL, 7, (int[]){6}), BAD_ARG);
  CHECK(sl_vector_insert(ctxt, NULL, 7, (int[]){6}), BAD_ARG);
  CHECK(sl_vector_insert(NULL, vec, 7, (int[]){6}), BAD_ARG);
  CHECK(sl_vector_insert(ctxt, vec, 7, (int[]){6}), BAD_ARG);
  CHECK(sl_vector_insert(NULL, NULL, 2, (int[]){6}), BAD_ARG);
  CHECK(sl_vector_insert(ctxt, NULL, 2, (int[]){6}), BAD_ARG);
  CHECK(sl_vector_insert(NULL, vec, 2, (int[]){6}), BAD_ARG);
  CHECK(sl_vector_insert(ctxt, vec, 2, (int[]){6}), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 7);
  CHECK(sl_vector_at(ctxt, vec, 0, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(ctxt, vec, 1, &data), OK);
  CHECK(*(int*)data, 1);
  CHECK(sl_vector_at(ctxt, vec, 2, &data), OK);
  CHECK(*(int*)data, 6);
  CHECK(sl_vector_at(ctxt, vec, 3, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(ctxt, vec, 4, &data), OK);
  CHECK(*(int*)data, 3);
  CHECK(sl_vector_at(ctxt, vec, 5, &data), OK);
  CHECK(*(int*)data, 4);
  CHECK(sl_vector_at(ctxt, vec, 6, &data), OK);
  CHECK(*(int*)data, 5);

  CHECK(sl_vector_insert(ctxt, vec, 6, (int[]){7}), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 8);
  CHECK(sl_vector_at(ctxt, vec, 0, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(ctxt, vec, 1, &data), OK);
  CHECK(*(int*)data, 1);
  CHECK(sl_vector_at(ctxt, vec, 2, &data), OK);
  CHECK(*(int*)data, 6);
  CHECK(sl_vector_at(ctxt, vec, 3, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(ctxt, vec, 4, &data), OK);
  CHECK(*(int*)data, 3);
  CHECK(sl_vector_at(ctxt, vec, 5, &data), OK);
  CHECK(*(int*)data, 4);
  CHECK(sl_vector_at(ctxt, vec, 6, &data), OK);
  CHECK(*(int*)data, 7);
  CHECK(sl_vector_at(ctxt, vec, 7, &data), OK);
  CHECK(*(int*)data, 5);

  CHECK(sl_vector_insert(ctxt, vec, 5, data), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 9);
  CHECK(sl_vector_at(ctxt, vec, 0, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(ctxt, vec, 1, &data), OK);
  CHECK(*(int*)data, 1);
  CHECK(sl_vector_at(ctxt, vec, 2, &data), OK);
  CHECK(*(int*)data, 6);
  CHECK(sl_vector_at(ctxt, vec, 3, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(ctxt, vec, 4, &data), OK);
  CHECK(*(int*)data, 3);
  CHECK(sl_vector_at(ctxt, vec, 5, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(ctxt, vec, 6, &data), OK);
  CHECK(*(int*)data, 4);
  CHECK(sl_vector_at(ctxt, vec, 7, &data), OK);
  CHECK(*(int*)data, 7);
  CHECK(sl_vector_at(ctxt, vec, 8, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(ctxt, vec, 9, &data), BAD_ARG);

  CHECK(sl_vector_push_back(ctxt, vec, (int[]){9}), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 10);
  CHECK(sl_vector_at(ctxt, vec, 9, &data), OK);
  CHECK(*(int*)data, 9);
  CHECK(sl_vector_insert(ctxt, vec, 0, data), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 11);

  CHECK(sl_vector_at(ctxt, vec, 0, &data), OK);
  CHECK(*(int*)data, 9);
  CHECK(sl_vector_at(ctxt, vec, 1, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(ctxt, vec, 2, &data), OK);
  CHECK(*(int*)data, 1);
  CHECK(sl_vector_at(ctxt, vec, 3, &data), OK);
  CHECK(*(int*)data, 6);
  CHECK(sl_vector_at(ctxt, vec, 4, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(ctxt, vec, 5, &data), OK);
  CHECK(*(int*)data, 3);
  CHECK(sl_vector_at(ctxt, vec, 6, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(ctxt, vec, 7, &data), OK);
  CHECK(*(int*)data, 4);
  CHECK(sl_vector_at(ctxt, vec, 8, &data), OK);
  CHECK(*(int*)data, 7);
  CHECK(sl_vector_at(ctxt, vec, 9, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(ctxt, vec, 10, &data), OK);
  CHECK(*(int*)data, 9);

  CHECK(sl_vector_remove(NULL, NULL, 12), BAD_ARG);
  CHECK(sl_vector_remove(ctxt, NULL, 12), BAD_ARG);
  CHECK(sl_vector_remove(NULL, vec, 12), BAD_ARG);
  CHECK(sl_vector_remove(ctxt, vec, 12), BAD_ARG);
  CHECK(sl_vector_remove(NULL, NULL, 5), BAD_ARG);
  CHECK(sl_vector_remove(ctxt, NULL, 5), BAD_ARG);
  CHECK(sl_vector_remove(NULL, vec, 5), BAD_ARG);
  CHECK(sl_vector_remove(ctxt, vec, 5), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 10);
  CHECK(sl_vector_at(ctxt, vec, 0, &data), OK);
  CHECK(*(int*)data, 9);
  CHECK(sl_vector_at(ctxt, vec, 1, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(ctxt, vec, 2, &data), OK);
  CHECK(*(int*)data, 1);
  CHECK(sl_vector_at(ctxt, vec, 3, &data), OK);
  CHECK(*(int*)data, 6);
  CHECK(sl_vector_at(ctxt, vec, 4, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(ctxt, vec, 5, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(ctxt, vec, 6, &data), OK);
  CHECK(*(int*)data, 4);
  CHECK(sl_vector_at(ctxt, vec, 7, &data), OK);
  CHECK(*(int*)data, 7);
  CHECK(sl_vector_at(ctxt, vec, 8, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(ctxt, vec, 9, &data), OK);
  CHECK(*(int*)data, 9);

  CHECK(sl_vector_remove(ctxt, vec, 9), OK);
  CHECK(sl_vector_remove(ctxt, vec, 0), OK);
  CHECK(sl_vector_remove(ctxt, vec, 2), OK);
  CHECK(sl_vector_remove(ctxt, vec, 5), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 6);
  CHECK(sl_vector_at(ctxt, vec, 0, &data), OK);
  CHECK(*(int*)data, 0);
  CHECK(sl_vector_at(ctxt, vec, 1, &data), OK);
  CHECK(*(int*)data, 1);
  CHECK(sl_vector_at(ctxt, vec, 2, &data), OK);
  CHECK(*(int*)data, 2);
  CHECK(sl_vector_at(ctxt, vec, 3, &data), OK);
  CHECK(*(int*)data, 5);
  CHECK(sl_vector_at(ctxt, vec, 4, &data), OK);
  CHECK(*(int*)data, 4);
  CHECK(sl_vector_at(ctxt, vec, 5, &data), OK);
  CHECK(*(int*)data, 5);

  CHECK(sl_vector_buffer(NULL, NULL, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_vector_buffer(ctxt, NULL, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_vector_buffer(NULL, vec, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_vector_buffer(ctxt, vec, NULL, NULL, NULL, NULL), OK);
  CHECK(sl_vector_buffer(ctxt, vec, &len, NULL, NULL, NULL), OK);
  CHECK(len, 6);
  CHECK(sl_vector_buffer(ctxt, vec, NULL, &size, NULL, NULL), OK);
  CHECK(size, sizeof(int));
  CHECK(sl_vector_buffer(ctxt, vec, NULL, NULL, &alignment, NULL), OK);
  CHECK(alignment, ALIGNOF(int));
  CHECK(sl_vector_buffer(ctxt, vec, &len, &size, &alignment, &data), OK);
  CHECK(len, 6);
  CHECK(size, sizeof(int));
  CHECK(alignment, ALIGNOF(int));
  CHECK(((int*)data)[0], 0);
  CHECK(((int*)data)[1], 1);
  CHECK(((int*)data)[2], 2);
  CHECK(((int*)data)[3], 5);
  CHECK(((int*)data)[4], 4);
  CHECK(((int*)data)[5], 5);

  CHECK(sl_vector_resize(NULL, NULL, 9, NULL), BAD_ARG);
  CHECK(sl_vector_resize(ctxt, NULL, 9, NULL), BAD_ARG);
  CHECK(sl_vector_resize(NULL, vec, 9, NULL), BAD_ARG);
  CHECK(sl_vector_resize(NULL, NULL, 9, (int[]){-1}), BAD_ARG);
  CHECK(sl_vector_resize(ctxt, NULL, 9, (int[]){-1}), BAD_ARG);
  CHECK(sl_vector_resize(NULL, vec, 9, (int[]){-1}), BAD_ARG);
  CHECK(sl_vector_resize(ctxt, vec, 9, (int[]){-1}), OK);
  CHECK(sl_vector_buffer(ctxt, vec, &len, NULL, NULL, &data), OK);
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

  CHECK(sl_vector_resize(ctxt, vec, 17, (int[]){-2}), OK);
  CHECK(sl_vector_buffer(ctxt, vec, &len, NULL, NULL, &data), OK);
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

  CHECK(sl_vector_resize(ctxt, vec, 20, NULL), OK);
  CHECK(sl_vector_buffer(ctxt, vec, &len, NULL, NULL, &data), OK);
  CHECK(len, 20);
  CHECK(((int*)data)[15], -2);
  CHECK(((int*)data)[16], -2);
  CHECK(((int*)data)[17], 0);
  CHECK(((int*)data)[18], 0);
  CHECK(((int*)data)[19], 0);

  CHECK(sl_vector_resize(ctxt, vec, 7, NULL), OK);
  CHECK(sl_vector_buffer(ctxt, vec, &len, NULL, NULL, &data), OK);
  CHECK(len, 7);
  CHECK(((int*)data)[0], 0);
  CHECK(((int*)data)[1], 1);
  CHECK(((int*)data)[2], 2);
  CHECK(((int*)data)[3], 5);
  CHECK(((int*)data)[4], 4);
  CHECK(((int*)data)[5], 5);
  CHECK(((int*)data)[6], -1);
 
  CHECK(sl_clear_vector(ctxt, vec), OK);
  CHECK(sl_vector_insert(ctxt, vec, 1, (int[]){1}), BAD_ARG);
  CHECK(sl_vector_buffer(ctxt, vec, &len, NULL, NULL, &data), OK);
  CHECK(len, 0);
  CHECK(data, NULL);

  CHECK(sl_vector_insert(ctxt, vec, 0, (int[]){1}), OK);
  CHECK(sl_vector_insert(ctxt, vec, 1, (int[]){2}), OK);
  CHECK(sl_vector_insert(ctxt, vec, 2, (int[]){4}), OK);
  CHECK(sl_vector_insert(ctxt, vec, 0, (int[]){0}), OK);
  CHECK(sl_vector_insert(ctxt, vec, 3, (int[]){3}), OK);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 5);
  CHECK(sl_vector_buffer(ctxt, vec, &len, NULL, NULL, &data), OK);
  CHECK(len, 5);
  CHECK(((int*)data)[0], 0);
  CHECK(((int*)data)[1], 1);
  CHECK(((int*)data)[2], 2);
  CHECK(((int*)data)[3], 3);
  CHECK(((int*)data)[4], 4);

  CHECK(sl_free_vector(NULL, NULL), BAD_ARG);
  CHECK(sl_free_vector(ctxt, NULL), BAD_ARG);
  CHECK(sl_free_vector(NULL, vec), BAD_ARG);
  CHECK(sl_free_vector(ctxt, vec), OK);

  CHECK(sl_create_vector(ctxt, sizeof(int), 16, &vec), OK);

  CHECK(sl_vector_capacity(NULL, NULL, NULL), BAD_ARG);
  CHECK(sl_vector_capacity(ctxt, NULL, NULL), BAD_ARG);
  CHECK(sl_vector_capacity(NULL, vec, NULL), BAD_ARG);
  CHECK(sl_vector_capacity(ctxt, vec, NULL), BAD_ARG);
  CHECK(sl_vector_capacity(NULL, NULL, &len), BAD_ARG);
  CHECK(sl_vector_capacity(ctxt, NULL, &len), BAD_ARG);
  CHECK(sl_vector_capacity(NULL, vec, &len), BAD_ARG);
  CHECK(sl_vector_capacity(ctxt, vec, &len), OK);
  CHECK(len, 0);

  CHECK(sl_vector_reserve(NULL, NULL, 0), BAD_ARG);
  CHECK(sl_vector_reserve(ctxt, NULL, 0), BAD_ARG);
  CHECK(sl_vector_reserve(NULL, vec, 0), BAD_ARG);
  CHECK(sl_vector_reserve(ctxt, vec, 0), OK);
  CHECK(sl_vector_capacity(ctxt, vec, &len), OK);
  CHECK(len, 0);
  CHECK(sl_vector_reserve(ctxt, vec, 4), OK);
  CHECK(sl_vector_capacity(ctxt, vec, &len), OK);
  CHECK(len, 4);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 0);
  CHECK(sl_vector_reserve(ctxt, vec, 2), OK);
  CHECK(sl_vector_capacity(ctxt, vec, &len), OK);
  CHECK(len, 4);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 0);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){0}), OK);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){1}), OK);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){2}), OK);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){3}), OK);
  CHECK(sl_vector_capacity(ctxt, vec, &len), OK);
  CHECK(len, 4);
  CHECK(sl_vector_length(ctxt, vec, &len), OK);
  CHECK(len, 4);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){4}), OK);
  CHECK(sl_vector_capacity(ctxt, vec, &len), OK);
  NCHECK(len, 4);

  CHECK(sl_vector_push_back(ctxt, vec, i + 1), SL_ALIGNMENT_ERROR);
  CHECK(sl_vector_push_back(ctxt, vec, (int[]){1}), OK);
  CHECK(sl_vector_insert(ctxt, vec, 0, i + 1), SL_ALIGNMENT_ERROR);
  CHECK(sl_free_vector(ctxt, vec), OK);

  CHECK(sl_free_context(ctxt), OK);

  return 0;
}
