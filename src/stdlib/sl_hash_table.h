#ifndef SL_HASH_TABLE_H
#define SL_HASH_TABLE_H

#include "stdlib/sl_error.h"
#include "stdlib/sl_pair.h"
#include <stdbool.h>
#include <stddef.h>

struct mem_allocator;
struct sl_hash_table;

struct sl_hash_table_it {
  struct sl_hash_table* hash_table;
  struct sl_pair pair;
  /*Private data. */
  struct entry* entry;
  size_t bucket;
};

extern enum sl_error
sl_create_hash_table
  (size_t key_size,
   size_t key_alignment,
   size_t data_size,
   size_t data_alignment,
   size_t (*hash_fcn)(const void*),
   bool (*eq_key)(const void*, const void*),
   struct mem_allocator* allocator, /* May be NULL. */
   struct sl_hash_table** out_hash_table);

extern enum sl_error
sl_free_hash_table
  (struct sl_hash_table* hash_table);

extern enum sl_error
sl_hash_table_insert
  (struct sl_hash_table* hash_table,
   const void* key,
   const void* data);

extern enum sl_error
sl_hash_table_erase
  (struct sl_hash_table* hash_table,
   const void* key,
   size_t* out_nb_erased); /* May be NULL. */

extern enum sl_error
sl_hash_table_find
  (struct sl_hash_table* hash_table,
   const void* key,
   void** data);

extern enum sl_error
sl_hash_table_find_pair
  (struct sl_hash_table* hash_table,
   const void* key,
   struct sl_pair* pair);

extern enum sl_error
sl_hash_table_data_count
  (struct sl_hash_table* hash_table,
   size_t *nb_data);

extern enum sl_error
sl_hash_table_resize
  (struct sl_hash_table* hash_table,
   size_t hint_nb_buckets);

extern enum sl_error
sl_hash_table_bucket_count
  (const struct sl_hash_table* hash_table,
   size_t* nb_buckets);

extern enum sl_error
sl_hash_table_used_bucket_count
  (const struct sl_hash_table* hash_table,
   size_t* nb_used_buckets);

extern enum sl_error
sl_hash_table_clear
  (struct sl_hash_table* hash_table);

extern enum sl_error
sl_hash_table_begin
  (struct sl_hash_table* hash_table,
   struct sl_hash_table_it* it,
   bool* is_end_reached);

extern enum sl_error
sl_hash_table_it_next
  (struct sl_hash_table_it* it,
   bool* is_end_reached);

/* Generic hash function. */
extern size_t
sl_hash
  (const void* data,
   size_t len);

#endif /* SL_HASH_TABLE_H */

