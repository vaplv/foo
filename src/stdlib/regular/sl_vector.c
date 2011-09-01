#include "stdlib/sl_vector.h"
#include "stdlib/sl.h"
#include "sys/sys.h"
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#define IS_MEMORY_OVERLAPPED(d0, sz0, d1, sz1) \
  ((uintptr_t)(d0) >= (uintptr_t)(d1) && \
   (uintptr_t)(d0) < (uintptr_t)(d1) + (sz1)) || \
  ((uintptr_t)(d0) + (sz0) >= (uintptr_t)(d1) && \
   (uintptr_t)(d0) + (sz0) < (uintptr_t)(d1) + (sz1))

struct sl_vector {
  size_t data_size;
  size_t data_alignment;
  size_t length;
  size_t capacity; /* In number of vector elements, not in bytes. */
  void* buffer;
};

/*******************************************************************************
 *
 * Implementation of the vector container.
 *
 ******************************************************************************/
EXPORT_SYM enum sl_error
sl_create_vector
  (struct sl_context* ctxt,
   size_t data_size,
   size_t data_alignment,
   struct sl_vector** out_vec)
{
  struct sl_vector* vec = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !out_vec || !data_size) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  if(!SL_IS_POWER_OF_2(data_alignment)) {
    err = SL_ALIGNMENT_ERROR;
    goto error;
  }

  vec = calloc(1, sizeof(struct sl_vector));
  if(vec == NULL) {
    err = SL_MEMORY_ERROR;
    goto error;
  }

  vec->data_size = data_size;
  vec->data_alignment = data_alignment;

exit:
  if(out_vec)
    *out_vec = vec;
  return err;

error:
  if(vec) {
    free(vec);
    vec = NULL;
  }
  goto exit;
}

EXPORT_SYM enum sl_error
sl_free_vector
  (struct sl_context* ctxt,
   struct sl_vector* vec)
{
  if(!ctxt || !vec)
    return SL_INVALID_ARGUMENT;

  if(vec->buffer)
    free(vec->buffer);

  free(vec);

  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_clear_vector
  (struct sl_context* ctxt,
   struct sl_vector* vec)
{
  if(!ctxt || !vec)
    return SL_INVALID_ARGUMENT;

  vec->length = 0;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_vector_push_back
  (struct sl_context* ctxt,
   struct sl_vector* vec,
   const void* data)
{
  size_t new_capacity = 0;
  void* buffer = NULL;
  void* ptr = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !vec || !data) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  if(!IS_ALIGNED(data, vec->data_alignment)) {
    err = SL_ALIGNMENT_ERROR;
    goto error;
  }

  if(vec->length == SIZE_MAX) {
    err = SL_OVERFLOW_ERROR;
    goto error;
  }

  assert(vec->length <= vec->capacity);
  if(vec->length == vec->capacity) {
    new_capacity = vec->capacity == 0 ? 1 : vec->capacity * 2;
    buffer = memalign(vec->data_alignment, new_capacity * vec->data_size);
    if(!buffer) {
      err = SL_MEMORY_ERROR;
      goto error;
    }

    buffer = memcpy(buffer, vec->buffer, vec->length * vec->data_size);

    if(vec->buffer)
      free(vec->buffer);

    vec->buffer = buffer;
    vec->capacity = new_capacity;
    buffer = NULL;
  }

  ptr = (void*)((uintptr_t)vec->buffer + vec->length * vec->data_size);
  memcpy(ptr, data, vec->data_size);
  ++vec->length;

exit:
  return err;

error:
  if(buffer)
    free(buffer);

  goto exit;
}

EXPORT_SYM enum sl_error
sl_vector_pop_back
  (struct sl_context* ctxt,
   struct sl_vector* vec)
{
  if(!ctxt || !vec)
    return SL_INVALID_ARGUMENT;

  vec->length -= (vec->length != 0);
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_vector_insert
  (struct sl_context* ctxt,
   struct sl_vector* vec,
   size_t id,
   const void* data)
{
  void* buffer = NULL;
  const void* src = NULL;
  void* dst = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !vec || (id > vec->length) || !data) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  if(!IS_ALIGNED(data, vec->data_alignment)) {
    err = SL_ALIGNMENT_ERROR;
    goto error;
  }

  if(vec->length == SIZE_MAX) {
    err = SL_OVERFLOW_ERROR;
    goto error;
  }

  if(id == vec->length) {
    err = sl_vector_push_back(ctxt, vec, data);
    if(err != SL_NO_ERROR)
      goto error;
  } else {
    if(vec->length == vec->capacity) {
      const size_t new_capacity = vec->capacity * 2;

      buffer = memalign(vec->data_alignment, new_capacity * vec->data_size);
      if(!buffer) {
        err = SL_MEMORY_ERROR;
        goto error;
      }

      /* Copy the vector data ranging from [0, id[ into the new buffer. */
      if(id > 0)
        memcpy(buffer, vec->buffer, vec->data_size * id);

      if(id < vec->length) {
        /* Copy from the vector data [id, length[ to the new buffer
         * [id+1, length + 1[. */
        src = (void*)((uintptr_t)(vec->buffer) + vec->data_size * id);
        dst = (void*)((uintptr_t)(buffer) + vec->data_size * (id + 1));
        dst = memcpy(dst, src, vec->data_size * (vec->length - id));
      }

      /* Set the src/dst pointer of the data insertion process. */
      dst = (void*)((uintptr_t)(buffer) + vec->data_size * id);
      src = data;
      dst = memcpy(dst, src, vec->data_size);

      /* The data to insert may be contained in vec, i.e. free vec->buffer
       * *AFTER* the insertion. */
      if(vec->buffer)
        free(vec->buffer);

      vec->buffer = buffer;
      vec->capacity = new_capacity;
      buffer = NULL;

    } else {
      if(id < vec->length) {
        src = (void*)((uintptr_t)(vec->buffer) + vec->data_size * id);
        dst = (void*)((uintptr_t)(vec->buffer) + vec->data_size * (id + 1));
        dst = memmove(dst, src, vec->data_size * (vec->length - id));
      }

      /* Set the src/dst pointer of the data insertion process. Note that If the
       * data to insert lies in the vector range [id, vec.length[ then it was
       * previously memoved. Its new address is offseted by data_size bytes. */
      dst = (void*)((uintptr_t)(vec->buffer) + vec->data_size * id);
      if(IS_MEMORY_OVERLAPPED
         (data,
          vec->data_size,
          (void*)((uintptr_t)(vec->buffer) + vec->data_size * id),
          (vec->length - id) * vec->data_size)) {
        src = (void*)((uintptr_t)data + vec->data_size);
      } else {
        src = data;
      }
      dst = memcpy(dst, src, vec->data_size);
    }
    ++vec->length;
  }

exit:
  return err;

error:
  if(buffer)
    free(buffer);
  goto exit;
}

EXPORT_SYM enum sl_error
sl_vector_remove
  (struct sl_context* ctxt,
   struct sl_vector* vec,
   size_t id)
{
  void* src = NULL;
  void* dst = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !vec || id >= vec->length) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  if(id != vec->length - 1) {
    src = (void*)((uintptr_t)vec->buffer + vec->data_size * (id + 1));
    dst = (void*)((uintptr_t)vec->buffer + vec->data_size * id);
    dst = memmove(dst, src, vec->data_size * (vec->length - id - 1));
  }
  --vec->length;

exit:
  return err;

error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_vector_resize
  (struct sl_context* ctxt,
   struct sl_vector* vec,
   size_t size,
   const void* data)
{
  void* buffer = NULL;
  size_t new_capacity = 0;
  size_t i = 0;
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !vec) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  if(size > vec->capacity) {
    for(new_capacity = vec->capacity; new_capacity < size; new_capacity *= 2);

    err = sl_vector_reserve(ctxt, vec, new_capacity);
    if(err != SL_NO_ERROR)
      goto error;
  }

  if(size > vec->length) {
    buffer = (void*)((uintptr_t)vec->buffer + vec->length * vec->data_size);
    if(data) {
      for(i = vec->length; i < size; ++i) {
        buffer = memcpy(buffer, data, vec->data_size);
        buffer = (void*)((uintptr_t)buffer + vec->data_size);
      }
    } else {
      memset(buffer, 0, (size - vec->length) * vec->data_size);
    }
  }
  vec->length = size;

exit:
  return err;

error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_vector_reserve
  (struct sl_context* ctxt,
   struct sl_vector* vec,
   size_t capacity)
{
  void* buffer = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !vec) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  if(capacity > vec->capacity) {
    buffer = memalign(vec->data_alignment, capacity * vec->data_size);
    if(!buffer) {
      err = SL_MEMORY_ERROR;
      goto error;
    }
    buffer = memcpy(buffer, vec->buffer, vec->length * vec->data_size);

    if(vec->buffer)
      free(vec->buffer);

    vec->buffer = buffer;
    vec->capacity = capacity;
    buffer = NULL;
  }

exit:
  return err;

error:
  if(buffer)
    free(buffer);
  goto exit;
}

EXPORT_SYM enum sl_error
sl_vector_capacity
  (struct sl_context* ctxt,
   struct sl_vector* vec,
   size_t* capacity)
{
  if(!ctxt || !vec || !capacity)
    return SL_INVALID_ARGUMENT;

  *capacity = vec->capacity;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_vector_at
  (struct sl_context* ctxt,
   struct sl_vector* vec,
   size_t id,
   void** data)
{
  if(!ctxt || !vec || (id >= vec->length) || !data)
    return SL_INVALID_ARGUMENT;

  *data = (void*)((uintptr_t)vec->buffer + id * vec->data_size);
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_vector_length
  (struct sl_context* ctxt,
   struct sl_vector* vec,
   size_t* length)
{
  if(!ctxt || !vec || !length)
    return SL_INVALID_ARGUMENT;

  *length = vec->length;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_vector_buffer
  (struct sl_context* ctxt,
   struct sl_vector* vec,
   size_t* length,
   size_t* data_size,
   size_t* data_alignment,
   void** buffer)
{
  if(!ctxt || !vec)
    return SL_INVALID_ARGUMENT;

  if(length)
    *length = vec->length;
  if(data_size)
    *data_size = vec->data_size;
  if(data_alignment)
    *data_alignment = vec->data_alignment;
  if(buffer)
    *buffer = vec->length == 0 ? NULL : vec->buffer;
  return SL_NO_ERROR;
}

