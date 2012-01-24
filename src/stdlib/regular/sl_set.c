#include "stdlib/sl_set.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct sl_set {
  int (*compare)(const void*, const void*);
  struct mem_allocator* allocator;
  struct sl_vector* vector;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
/* Let a vector vec, this function finds the index of the data in set and
 * defines if the data already lies into set. */
static enum sl_error
data_id
  (struct sl_set* set,
   const void* data,
   size_t* out_id,
   bool* out_is_data_found)
{
  size_t begin = 0;
  size_t end = 0;
  size_t len = 0;
  enum sl_error err = SL_NO_ERROR;
  bool is_data_found = false;

  assert(set && data && out_id && out_is_data_found);

  err = sl_vector_length(set->vector, &len);
  if(err != SL_NO_ERROR)
    goto error;

  begin = 0;
  end = len;

  while(begin != end && !is_data_found) {
    void* tmp_data = NULL;
    const size_t at = begin + (end - begin) / 2;
    int cmp = 0;

    err = sl_vector_at(set->vector, at, &tmp_data);
    if(err != SL_NO_ERROR)
      goto error;

    cmp = set->compare(data, tmp_data);
    if(cmp == 0) {
        is_data_found = true;
        end = at;
    } else {
      if(cmp > 0) {
        begin = at + 1;
      } else if(cmp < 0) {
        end = at;
      }
    }
  }

exit:
  *out_id = end;
  *out_is_data_found = is_data_found;
  return err;

error:
  end = 0;
  is_data_found = false;
  goto exit;
}

/*******************************************************************************
 *
 * Implementation of the sorted vector functions.
 *
 ******************************************************************************/
EXPORT_SYM enum sl_error
sl_create_set
  (size_t data_size,
   size_t data_alignment,
   int (*compare)(const void*, const void*),
   struct mem_allocator* specific_allocator,
   struct sl_set** out_vector)
{
  struct mem_allocator* allocator = NULL;
  struct sl_set* set = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!compare || !out_vector) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }
  allocator = specific_allocator ? specific_allocator : &mem_default_allocator;
  set = MEM_CALLOC(allocator, 1, sizeof(struct sl_set));
  if(!set) {
    err = SL_MEMORY_ERROR;
    goto error;
  }
  err = sl_create_vector
    (data_size, data_alignment, specific_allocator, &set->vector);
  if(err != SL_NO_ERROR)
    goto error;
  set->compare = compare;
  set->allocator = allocator;

exit:
  if(out_vector)
    *out_vector = set;
  return err;

error:
  if(set) {
    if(set->vector) {
      err = sl_free_vector(set->vector);
      assert(err == SL_NO_ERROR);
    }
    MEM_FREE(allocator, set);
    set = NULL;
  }
  goto exit;
}

EXPORT_SYM enum sl_error
sl_free_set
  (struct sl_set* set)
{
  struct mem_allocator* allocator = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!set) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }
  err = sl_free_vector(set->vector);
  if(err != SL_NO_ERROR)
    goto error;
  allocator = set->allocator;
  MEM_FREE(allocator, set);

exit:
  return err;
error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_clear_set
  (struct sl_set* set)
{
  if(!set)
    return SL_INVALID_ARGUMENT;

  return sl_clear_vector(set->vector);
}

EXPORT_SYM enum sl_error
sl_set_insert
  (struct sl_set* set,
   const void* data)
{
  size_t id = 0;
  enum sl_error err = SL_NO_ERROR;
  bool is_data_found = false;

  if(!set || !data)
    return SL_INVALID_ARGUMENT;

  err = data_id(set, data, &id, &is_data_found);
  if(err != SL_NO_ERROR)
    return err;

  if(is_data_found)
    return SL_INVALID_ARGUMENT;

  err = sl_vector_insert(set->vector, id, data);
  if(err != SL_NO_ERROR)
    return err;

  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_set_find
  (struct sl_set* set,
   const void* data,
   size_t* out_id)
{
  size_t id = 0;
  enum sl_error err = SL_NO_ERROR;
  bool is_data_found = false;

  if(!set || !data || !out_id) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  err = data_id(set, data, &id, &is_data_found);
  if(err != SL_NO_ERROR)
    goto error;

  if(is_data_found) {
    *out_id = id;
  } else {
    size_t len = 0;
    err = sl_vector_length(set->vector, &len);
    if(err != SL_NO_ERROR)
      goto error;

    *out_id = len;
  }

exit:
  return err;

error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_set_at
  (struct sl_set* set,
   size_t id,
   void** data)
{
  if(!set)
    return SL_INVALID_ARGUMENT;

  return sl_vector_at(set->vector, id, data);
}

EXPORT_SYM enum sl_error
sl_set_remove
  (struct sl_set* set,
   const void* data)
{
  size_t id = 0;
  enum sl_error err = SL_NO_ERROR;
  bool is_data_found = false;

  if(!set || !data) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  err = data_id(set, data, &id, &is_data_found);
  if(err != SL_NO_ERROR)
    goto error;

  if(!is_data_found) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  err = sl_vector_remove(set->vector, id);
  if(err != SL_NO_ERROR)
    goto error;

exit:
  return err;

error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_set_reserve
  (struct sl_set* set,
   size_t capacity)
{
  if(!set)
    return SL_INVALID_ARGUMENT;

  return sl_vector_reserve(set->vector, capacity);
}

EXPORT_SYM enum sl_error
sl_set_capacity
  (struct sl_set* set,
   size_t *out_capacity)
{
  if(!set)
    return SL_INVALID_ARGUMENT;

  return sl_vector_capacity(set->vector, out_capacity);
}

EXPORT_SYM enum sl_error
sl_set_length
  (struct sl_set* set,
   size_t* out_length)
{
  if(!set)
    return SL_INVALID_ARGUMENT;

  return sl_vector_length(set->vector, out_length);
}

EXPORT_SYM enum sl_error
sl_set_buffer
  (struct sl_set* set,
   size_t* length,
   size_t* data_size,
   size_t* data_alignment,
   void** buffer)
{
  if(!set)
    return SL_INVALID_ARGUMENT;

  return sl_vector_buffer
    (set->vector, length, data_size, data_alignment, buffer);
}

