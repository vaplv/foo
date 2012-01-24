#ifndef SL_SET_H
#define SL_SET_H

#include "stdlib/sl_error.h"
#include <stddef.h>

struct mem_allocator* allocator;
struct sl_set;

extern enum sl_error
sl_create_set
  (size_t data_size,
   size_t data_alignment,
   int (*data_comparator)(const void*, const void*),
   struct mem_allocator* allocator, /* May be NULL. */
   struct sl_set** set);

extern enum sl_error
sl_free_set
  (struct sl_set* set);

extern enum sl_error
sl_clear_set
  (struct sl_set* set);

extern enum sl_error
sl_set_insert
  (struct sl_set* set,
   const void* data);

extern enum sl_error
sl_set_find
  (struct sl_set* set,
   const void* data,
   size_t* out_id); /* set to set_length if the data is not found. */

extern enum sl_error
sl_set_at
  (struct sl_set* set,
   size_t id,
   void** data);

extern enum sl_error
sl_set_remove
  (struct sl_set* set,
   const void* data);

extern enum sl_error
sl_set_reserve
  (struct sl_set* set,
   size_t capacity);

extern enum sl_error
sl_set_capacity
  (struct sl_set* set,
   size_t* out_capacity);

extern enum sl_error
sl_set_length
  (struct sl_set* set,
   size_t* out_length);

extern enum sl_error
sl_set_buffer
  (struct sl_set* set,
   size_t* out_length,
   size_t* out_data_size,
   size_t* out_data_alignment,
   void** out_buffer);

#endif /* SL_SET_H */

