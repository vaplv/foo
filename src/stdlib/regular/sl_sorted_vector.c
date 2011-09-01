#include "stdlib/sl_sorted_vector.h"
#include "stdlib/sl_vector.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct sl_sorted_vector {
  int (*compare)(const void*, const void*);
  struct sl_vector* vector;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/

/* Let a vector vec, this function finds the index of the data in vec and
 * defines if the data already lies into vec. */
static enum sl_error
data_id
  (struct sl_context* ctxt,
   struct sl_sorted_vector* svec,
   const void* data,
   size_t* out_id,
   bool* out_is_data_found)
{
  size_t begin = 0;
  size_t end = 0;
  size_t len = 0;
  enum sl_error err = SL_NO_ERROR;
  bool is_data_found = false;

  assert(ctxt && svec && data && out_id && out_is_data_found);

  err = sl_vector_length(ctxt, svec->vector, &len);
  if(err != SL_NO_ERROR)
    goto error;

  begin = 0;
  end = len;

  while(begin != end && !is_data_found) {
    void* tmp_data = NULL;
    const size_t at = begin + (end - begin) / 2;
    int cmp = 0;

    err = sl_vector_at(ctxt, svec->vector, at, &tmp_data);
    if(err != SL_NO_ERROR)
      goto error;

    cmp = svec->compare(data, tmp_data);
    if(cmp == 0) {
        is_data_found = true;
        end = at;
    } else {
      if(cmp > 0) {
        begin = at + 1;
      } else if (cmp < 0) {
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
sl_create_sorted_vector
  (struct sl_context* ctxt,
   size_t data_size,
   size_t data_alignment,
   int (*compare)(const void*, const void*),
   struct sl_sorted_vector** out_vector)
{
  struct sl_sorted_vector* svec = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!compare || !out_vector) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  svec = calloc(1, sizeof(struct sl_sorted_vector));
  if(!svec) {
    err = SL_MEMORY_ERROR;
    goto error;
  }

  err = sl_create_vector(ctxt, data_size, data_alignment, &svec->vector);
  if(err != SL_NO_ERROR)
    goto error;

  svec->compare = compare;

exit:
  if(out_vector)
    *out_vector = svec;
  return err;

error:
  if(svec) {
    if(svec->vector) {
      err = sl_free_vector(ctxt, svec->vector);
      assert(err == SL_NO_ERROR);
    }
    free(svec);
    svec = NULL;
  }
  goto exit;
}

EXPORT_SYM enum sl_error
sl_free_sorted_vector
  (struct sl_context* ctxt,
   struct sl_sorted_vector* svec)
{
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !svec) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  err = sl_free_vector(ctxt, svec->vector);
  if(err != SL_NO_ERROR)
    goto error;

  free(svec);

exit:
  return err;

error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_clear_sorted_vector
  (struct sl_context* ctxt,
   struct sl_sorted_vector* svec)
{
  if(!svec)
    return SL_INVALID_ARGUMENT;

  return sl_clear_vector(ctxt, svec->vector);
}

EXPORT_SYM enum sl_error
sl_sorted_vector_insert
  (struct sl_context* ctxt,
   struct sl_sorted_vector* svec,
   const void* data)
{
  size_t id = 0;
  enum sl_error err = SL_NO_ERROR;
  bool is_data_found = false;

  if(!ctxt || !svec || !data)
    return SL_INVALID_ARGUMENT;

  err = data_id(ctxt, svec, data, &id, &is_data_found);
  if(err != SL_NO_ERROR)
    return err;

  if(is_data_found)
    return SL_INVALID_ARGUMENT;

  err = sl_vector_insert(ctxt, svec->vector, id, data);
  if(err != SL_NO_ERROR)
    return err;

  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_sorted_vector_find
  (struct sl_context* ctxt,
   struct sl_sorted_vector* svec,
   const void* data,
   size_t* out_id)
{
  size_t id = 0;
  enum sl_error err = SL_NO_ERROR;
  bool is_data_found = false;

  if(!ctxt || !svec || !data || !out_id) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  err = data_id(ctxt, svec, data, &id, &is_data_found);
  if(err != SL_NO_ERROR)
    goto error;

  if(is_data_found) {
    *out_id = id;
  } else {
    size_t len = 0;
    err = sl_vector_length(ctxt, svec->vector, &len);
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
sl_sorted_vector_at
  (struct sl_context* ctxt,
   struct sl_sorted_vector* svec,
   size_t id,
   void** data)
{
  if(!svec)
    return SL_INVALID_ARGUMENT;

  return sl_vector_at(ctxt, svec->vector, id, data);
}

EXPORT_SYM enum sl_error
sl_sorted_vector_remove
  (struct sl_context* ctxt,
   struct sl_sorted_vector* svec,
   const void* data)
{
  size_t id = 0;
  enum sl_error err = SL_NO_ERROR;
  bool is_data_found = false;

  if(!ctxt || !svec || !data) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  err = data_id(ctxt, svec, data, &id, &is_data_found);
  if(err != SL_NO_ERROR)
    goto error;

  if(!is_data_found) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  err = sl_vector_remove(ctxt, svec->vector, id);
  if(err != SL_NO_ERROR)
    goto error;

exit:
  return err;

error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_sorted_vector_reserve
  (struct sl_context* ctxt,
   struct sl_sorted_vector* svec,
   size_t capacity)
{
  if(!svec)
    return SL_INVALID_ARGUMENT;

  return sl_vector_reserve(ctxt, svec->vector, capacity);
}

EXPORT_SYM enum sl_error
sl_sorted_vector_capacity
  (struct sl_context* ctxt,
   struct sl_sorted_vector* svec,
   size_t *out_capacity)
{
  if(!svec)
    return SL_INVALID_ARGUMENT;

  return sl_vector_capacity(ctxt, svec->vector, out_capacity);
}

EXPORT_SYM enum sl_error
sl_sorted_vector_length
  (struct sl_context* ctxt,
   struct sl_sorted_vector* svec,
   size_t* out_length)
{
  if(!svec)
    return SL_INVALID_ARGUMENT;

  return sl_vector_length(ctxt, svec->vector, out_length);
}

EXPORT_SYM enum sl_error
sl_sorted_vector_buffer
  (struct sl_context* ctxt,
   struct sl_sorted_vector* svec,
   size_t* length,
   size_t* data_size,
   size_t* data_alignment,
   void** buffer)
{
  if(!svec)
    return SL_INVALID_ARGUMENT;

  return sl_vector_buffer
    (ctxt, svec->vector, length, data_size, data_alignment, buffer);
}

