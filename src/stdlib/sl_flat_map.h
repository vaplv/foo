#ifndef SL_FLAT_MAP
#define SL_FLAT_MAP

#include "stdlib/sl_error.h"
#include <stddef.h>

struct mem_allocator;
struct sl_flat_map;
struct sl_pair;

extern enum sl_error
sl_create_flat_map
  (size_t key_size,
   size_t key_alignment,
   size_t data_size,
   size_t data_alignment,
   int (*cmp_key)(const void*, const void*),
   struct mem_allocator* allocator, /* May be NULL. */
   struct sl_flat_map** out_map);

extern enum sl_error
sl_free_flat_map
  (struct sl_flat_map* map);

extern enum sl_error
sl_clear_flat_map
  (struct sl_flat_map* map);

extern enum sl_error
sl_flat_map_insert
  (struct sl_flat_map* map,
   const void* key,
   const void* data,
   size_t* insert_id); /* May be NULL. */

extern enum sl_error
sl_flat_map_erase
  (struct sl_flat_map* map,
   const void* key,
   size_t* erase_id); /* May be NULL. */

extern enum sl_error
sl_flat_map_find
  (struct sl_flat_map* map,
   const void* key,
   void** data);

extern enum sl_error
sl_flat_map_clear
  (struct sl_flat_map* map);

extern enum sl_error
sl_flat_map_lower_bound
  (struct sl_flat_map* map,
   const void* key,
   size_t* lower_bound);

extern enum sl_error
sl_flat_map_upper_bound
  (struct sl_flat_map* map,
   const void* key,
   size_t* upper_bound);

extern enum sl_error
sl_flat_map_length
  (struct sl_flat_map* map,
   size_t* out_length);

extern enum sl_error
sl_flat_map_at
  (struct sl_flat_map* map,
   size_t at,
   struct sl_pair* pair);

extern enum sl_error
sl_flat_map_key_buffer
  (struct sl_flat_map* map,
   size_t* length, /* May be NULL. */
   size_t* key_size, /* May be NULL. */
   size_t* key_alignment, /* May be NULL. */
   void** key_buffer); /* May be NULL. */

extern enum sl_error
sl_flat_map_data_buffer
  (struct sl_flat_map* map,
   size_t* length, /* May be NULL. */
   size_t* data_size, /* May be NULL. */
   size_t* data_alignment, /* May be NULL. */
   void** data_buffer); /* May be NULL. */

#endif /* SL_FLAT_MAP */

