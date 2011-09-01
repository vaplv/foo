#ifndef SL_SORTED_VECTOR_H
#define SL_SORTED_VECTOR_H

#include "stdlib/sl_error.h"
#include <stddef.h>

struct sl_context;
struct sl_sorted_vector;

extern enum sl_error
sl_create_sorted_vector
  (struct sl_context* ctxt,
   size_t data_size,
   size_t data_alignment,
   int (*data_comparator)(const void*, const void*),
   struct sl_sorted_vector** out_vector);

extern enum sl_error
sl_free_sorted_vector
  (struct sl_context* ctxt,
   struct sl_sorted_vector* vector);

extern enum sl_error
sl_clear_sorted_vector
  (struct sl_context* ctxt,
   struct sl_sorted_vector* vector);

extern enum sl_error
sl_sorted_vector_insert
  (struct sl_context* ctxt,
   struct sl_sorted_vector* vector,
   const void* data);

extern enum sl_error
sl_sorted_vector_find
  (struct sl_context* ctxt,
   struct sl_sorted_vector* vector,
   const void* data,
   size_t* out_id); /* set to vector_length if the data is not found. */

extern enum sl_error
sl_sorted_vector_at
  (struct sl_context* ctxt,
   struct sl_sorted_vector* vector,
   size_t id,
   void** data);

extern enum sl_error
sl_sorted_vector_remove
  (struct sl_context* ctxt,
   struct sl_sorted_vector* vector,
   const void* data);

extern enum sl_error
sl_sorted_vector_reserve
  (struct sl_context* ctxt,
   struct sl_sorted_vector* vector,
   size_t capacity);

extern enum sl_error
sl_sorted_vector_capacity
  (struct sl_context* ctxt,
   struct sl_sorted_vector* vector,
   size_t* out_capacity);

extern enum sl_error
sl_sorted_vector_length
  (struct sl_context* ctxt,
   struct sl_sorted_vector* vector,
   size_t* out_length);

extern enum sl_error
sl_sorted_vector_buffer
  (struct sl_context* ctxt,
   struct sl_sorted_vector* vector,
   size_t* out_length,
   size_t* out_data_size,
   size_t* out_data_alignment,
   void** out_buffer);

#endif /* SL_SORTED_VECTOR_H */

