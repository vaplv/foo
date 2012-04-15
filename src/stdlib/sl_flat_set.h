#ifndef SL_FLAT_SET_H
#define SL_FLAT_SET_H

#include "stdlib/sl_error.h"
#include <stddef.h>

struct mem_allocator* allocator;

/* Associative container in which the elements themselves are the key. */
struct sl_flat_set;

extern enum sl_error
sl_create_flat_set
  (size_t data_size,
   size_t data_alignment,
   int (*data_comparator)(const void*, const void*),
   struct mem_allocator* allocator, /* May be NULL. */
   struct sl_flat_set** set);

extern enum sl_error
sl_free_flat_set
  (struct sl_flat_set* set);

extern enum sl_error
sl_clear_flat_set
  (struct sl_flat_set* set);

extern enum sl_error
sl_flat_set_insert
  (struct sl_flat_set* set,
   const void* data,
   size_t* insert_id); /* May be NULL. */

extern enum sl_error
sl_flat_set_erase
  (struct sl_flat_set* set,
   const void* data,
   size_t* erase_id); /* May be NULL. */

extern enum sl_error
sl_flat_set_find
  (struct sl_flat_set* set,
   const void* data,
   size_t* out_id); /* set to set_length if the data is not found. */

extern enum sl_error
sl_flat_set_at
  (struct sl_flat_set* set,
   size_t id,
   void** data);

extern enum sl_error
sl_flat_set_reserve
  (struct sl_flat_set* set,
   size_t capacity);

extern enum sl_error
sl_flat_set_capacity
  (struct sl_flat_set* set,
   size_t* out_capacity);

extern enum sl_error
sl_flat_set_length
  (struct sl_flat_set* set,
   size_t* out_length);

extern enum sl_error
sl_flat_set_lower_bound
  (struct sl_flat_set* set,
   const void* data,
   size_t* lower_bound);

extern enum sl_error
sl_flat_set_upper_bound
  (struct sl_flat_set* set,
   const void* data,
   size_t* upper_bound);

extern enum sl_error
sl_flat_set_buffer
  (struct sl_flat_set* set,
   size_t* out_length,
   size_t* out_data_size,
   size_t* out_data_alignment,
   void** out_buffer);

#endif /* SL_FLAT_SET_H */

