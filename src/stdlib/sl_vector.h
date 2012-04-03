#ifndef SL_VECTOR_H
#define SL_VECTOR_H

#include "stdlib/sl_error.h"
#include <stddef.h>

struct mem_allocator;
struct sl_vector;

extern enum sl_error
sl_create_vector
  (size_t data_size,
   size_t data_alignment,
   struct mem_allocator* allocator, /* May be NULL. */
   struct sl_vector** out_vector);

extern enum sl_error
sl_free_vector
  (struct sl_vector* vector);

extern enum sl_error
sl_clear_vector
  (struct sl_vector* vector);

extern enum sl_error
sl_vector_push_back
  (struct sl_vector* vector,
   const void* data);

extern enum sl_error
sl_vector_push_back_n
  (struct sl_vector* vector,
   size_t count,
   const void* data);

extern enum sl_error
sl_vector_pop_back
  (struct sl_vector* vector);

extern enum sl_error
sl_vector_insert
  (struct sl_vector* vector,
   size_t id,
   const void* data);

extern enum sl_error
sl_vector_insert_n
  (struct sl_vector* vector,
   size_t id,
   size_t count,
   const void* data);

extern enum sl_error
sl_vector_remove
  (struct sl_vector* vector,
   size_t id);

extern enum sl_error
sl_vector_remove_n
  (struct sl_vector* vector,
   size_t id,
   size_t count);

extern enum sl_error
sl_vector_resize
  (struct sl_vector* vector,
   size_t size,
   const void* data); /* May be NULL. */

extern enum sl_error
sl_vector_reserve
  (struct sl_vector* vector,
   size_t capacity);

extern enum sl_error
sl_vector_capacity
  (struct sl_vector* vector,
   size_t* out_capacity);

extern enum sl_error
sl_vector_at
  (struct sl_vector* vector,
   size_t id,
   void** out_data);

extern enum sl_error
sl_vector_length
  (struct sl_vector* vector,
   size_t* out_length);

extern enum sl_error
sl_vector_buffer
  (struct sl_vector* vector,
   size_t* out_length,
   size_t* out_data_size,
   size_t* out_data_alignment,
   void** out_buffer);

#endif /* SL_VECTOR_H */

