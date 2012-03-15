#include "stdlib/regular/sl_c.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct sl_vector {
  struct mem_allocator* allocator;
  size_t data_size;
  size_t data_alignment;
  size_t length;
  size_t capacity; /* In number of vector elements, not in bytes. */
  void* buffer;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static enum sl_error
ensure_allocated(struct sl_vector* vec, size_t capacity, bool keep_data)
{
  void* buffer = NULL;
  enum sl_error sl_err = SL_NO_ERROR;
  assert(vec);

  if(capacity > vec->capacity) {
    size_t new_capacity = 0;
    SL_NEXT_POWER_OF_2(capacity, new_capacity);
    buffer = MEM_ALIGNED_ALLOC
      (vec->allocator, new_capacity * vec->data_size, vec->data_alignment);
    if(!buffer) {
      sl_err = SL_MEMORY_ERROR;
      goto error;
    }
    if(keep_data) {
      buffer = memcpy(buffer, vec->buffer, vec->capacity * vec->data_size);
    }
    MEM_FREE(vec->allocator, vec->buffer);
    vec->buffer = buffer;
    vec->capacity = new_capacity;
    buffer = NULL;
  }

exit:
  return sl_err;
error:
  if(buffer) {
    MEM_FREE(vec->allocator, buffer);
    buffer = NULL;
  }
  goto exit;
}

/*******************************************************************************
 *
 * Implementation of the vector container.
 *
 ******************************************************************************/
EXPORT_SYM enum sl_error
sl_create_vector
  (size_t data_size,
   size_t data_alignment,
   struct mem_allocator* specific_allocator,
   struct sl_vector** out_vec)
{
  struct mem_allocator* allocator = NULL;
  struct sl_vector* vec = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!out_vec || !data_size) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }
  if(!SL_IS_POWER_OF_2(data_alignment)) {
    err = SL_ALIGNMENT_ERROR;
    goto error;
  }
  allocator = specific_allocator ? specific_allocator : &mem_default_allocator;
  vec = MEM_CALLOC(allocator, 1, sizeof(struct sl_vector));
  if(vec == NULL) {
    err = SL_MEMORY_ERROR;
    goto error;
  }
  vec->allocator = allocator;
  vec->data_size = data_size;
  vec->data_alignment = data_alignment;

exit:
  if(out_vec)
    *out_vec = vec;
  return err;

error:
  if(vec) {
    assert(allocator);
    MEM_FREE(allocator, vec);
    vec = NULL;
  }
  goto exit;
}

EXPORT_SYM enum sl_error
sl_free_vector
  (struct sl_vector* vec)
{
  struct mem_allocator* allocator = NULL;

  if(!vec)
    return SL_INVALID_ARGUMENT;

  allocator = vec->allocator;
  if(vec->buffer)
    MEM_FREE(allocator, vec->buffer);
  MEM_FREE(allocator, vec);

  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_clear_vector
  (struct sl_vector* vec)
{
  if(!vec)
    return SL_INVALID_ARGUMENT;
  vec->length = 0;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_vector_push_back
  (struct sl_vector* vec,
   const void* data)
{
  void* ptr = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!vec || !data) {
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
  err = ensure_allocated(vec, vec->length + 1, true);
  if(SL_NO_ERROR != err)
    goto error;
  ptr = (void*)((uintptr_t)vec->buffer + vec->length * vec->data_size);
  memcpy(ptr, data, vec->data_size);
  ++vec->length;

exit:
  return err;
error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_vector_pop_back
  (struct sl_vector* vec)
{
  if(!vec)
    return SL_INVALID_ARGUMENT;
  vec->length -= (vec->length != 0);
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_vector_insert
  (struct sl_vector* vec,
   size_t id,
   const void* data)
{
  return sl_vector_insert_n(vec, id, 1, data);
}

EXPORT_SYM enum sl_error
sl_vector_insert_n
  (struct sl_vector* vec,
   size_t id,
   size_t count,
   const void* data)
{
  void* buffer = NULL;
  const void* src = NULL;
  void* dst = NULL;
  size_t i = 0;
  enum sl_error err = SL_NO_ERROR;

  if(!vec || (id > vec->length) || !data) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }
  if(0 == count) {
    goto exit;
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
    err = ensure_allocated(vec, vec->length + count, true);
    if(err != SL_NO_ERROR)
      goto error;

    dst = (void*)((uintptr_t)(vec->buffer) + vec->data_size * id);
    src = data;
    for(i = 0; i < count; ++i) {
      dst = memcpy(dst, src, vec->data_size);
      dst = (void*)((uintptr_t)(dst) + vec->data_size);
    }
  } else {
    if(vec->length + count >= vec->capacity) {
      size_t new_capacity = 0;
      SL_NEXT_POWER_OF_2(vec->length + count, new_capacity);

      buffer = MEM_ALIGNED_ALLOC
        (vec->allocator, new_capacity * vec->data_size, vec->data_alignment);
      if(!buffer) {
        err = SL_MEMORY_ERROR;
        goto error;
      }

      /* Copy the vector data ranging from [0, id[ into the new buffer. */
      if(id > 0)
        memcpy(buffer, vec->buffer, vec->data_size * id);

      if(id < vec->length) {
        /* Copy from the vector data [id, length[ to the new buffer
         * [id+count, length + count[. */
        src = (void*)((uintptr_t)(vec->buffer) + vec->data_size * id);
        dst = (void*)((uintptr_t)(buffer) + vec->data_size * (id + count));
        dst = memcpy(dst, src, vec->data_size * (vec->length - id));
      }

      /* Set the src/dst pointer of the data insertion process. */
      dst = (void*)((uintptr_t)(buffer) + vec->data_size * id);
      src = data;
      for(i = 0; i < count; ++i) {
        dst = memcpy(dst, src, vec->data_size);
        dst = (void*)((uintptr_t)(dst) + vec->data_size);
      }

      /* The data to insert may be contained in vec, i.e. free vec->buffer
       * *AFTER* the insertion. */
      if(vec->buffer)
        MEM_FREE(vec->allocator, vec->buffer);

      vec->buffer = buffer;
      vec->capacity = new_capacity;
      buffer = NULL;

    } else {
      if(id < vec->length) {
        src = (void*)((uintptr_t)(vec->buffer) + vec->data_size * id);
        dst = (void*)((uintptr_t)(vec->buffer) + vec->data_size * (id + count));
        dst = memmove(dst, src, vec->data_size * (vec->length - id));
      }

      /* Set the src/dst pointer of the data insertion process. Note that If the
       * data to insert lies in the vector range [id, vec.length[ then it was
       * previously memoved. Its new address is offseted by count * data_size
       * bytes. */
      dst = (void*)((uintptr_t)(vec->buffer) + vec->data_size * id);
      if(IS_MEMORY_OVERLAPPED
         (data,
          vec->data_size,
          (void*)((uintptr_t)(vec->buffer) + vec->data_size * id),
          (vec->length - id) * vec->data_size)) {
        src = (void*)((uintptr_t)data + count * vec->data_size);
      } else {
        src = data;
      }
      for(i = 0; i < count; ++i) {
        dst = memcpy(dst, src, vec->data_size);
        dst = (void*)((uintptr_t)(dst) + vec->data_size);
      }
    }
  }
  vec->length += count;

exit:
  return err;
error:
  if(buffer)
    MEM_FREE(vec->allocator, buffer);
  goto exit;

}

EXPORT_SYM enum sl_error
sl_vector_remove
  (struct sl_vector* vec,
   size_t id)
{
  void* src = NULL;
  void* dst = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!vec || id >= vec->length) {
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
  (struct sl_vector* vec,
   size_t size,
   const void* data)
{
  void* buffer = NULL;
  size_t new_capacity = 0;
  size_t i = 0;
  enum sl_error err = SL_NO_ERROR;

  if(!vec) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }
  if(size > vec->capacity) {
    for(new_capacity = vec->capacity; new_capacity < size; new_capacity *= 2);

    err = ensure_allocated(vec, new_capacity, true);
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
  (struct sl_vector* vec,
   size_t capacity)
{
  void* buffer = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!vec) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }
  err = ensure_allocated(vec, capacity, true);

exit:
  return err;
error:
  if(buffer)
    MEM_FREE(vec->allocator, buffer);
  goto exit;
}

EXPORT_SYM enum sl_error
sl_vector_capacity
  (struct sl_vector* vec,
   size_t* capacity)
{
  if(!vec || !capacity)
    return SL_INVALID_ARGUMENT;

  *capacity = vec->capacity;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_vector_at
  (struct sl_vector* vec,
   size_t id,
   void** data)
{
  if(!vec || (id >= vec->length) || !data)
    return SL_INVALID_ARGUMENT;

  *data = (void*)((uintptr_t)vec->buffer + id * vec->data_size);
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_vector_length
  (struct sl_vector* vec,
   size_t* length)
{
  if(!vec || !length)
    return SL_INVALID_ARGUMENT;

  *length = vec->length;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_vector_buffer
  (struct sl_vector* vec,
   size_t* length,
   size_t* data_size,
   size_t* data_alignment,
   void** buffer)
{
  enum sl_error err = SL_NO_ERROR;
  if(!vec) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }
  if(length)
    *length = vec->length;
  if(data_size)
    *data_size = vec->data_size;
  if(data_alignment)
    *data_alignment = vec->data_alignment;
  if(buffer) {
    if(!IS_ALIGNED(*buffer, vec->data_alignment)) {
      err = SL_ALIGNMENT_ERROR;
      goto error;
    }
    *buffer = vec->length == 0 ? NULL : vec->buffer;
  }

exit:
  return err;

error:
  if(length)
    *length = 0;
  if(data_size)
    *data_size = 0;
  if(data_alignment)
    *data_alignment = 0;
  if(buffer)
    *buffer = NULL;
  goto exit;
}

