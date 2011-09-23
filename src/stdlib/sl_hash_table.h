#ifndef SL_HASH_TABLE_H
#define SL_HASH_TABLE_H

#include "stdlib/sl_error.h"
#include <stddef.h>

struct sl_context;
struct sl_hash_table;

extern enum sl_error
sl_create_hash_table
  (struct sl_context* ctxt,
   size_t data_size,
   size_t data_alignment,
   size_t key_size,
   int (*compare)(const void*, const void*),
   const void* (*get_key)(const void*),
   struct sl_hash_table** out_hash_table);

extern enum sl_error
sl_free_hash_table
  (struct sl_context* ctxt,
   struct sl_hash_table* hash_table);

extern enum sl_error
sl_hash_table_insert
  (struct sl_context* ctxt,
   struct sl_hash_table* hash_table,
   const void* data);

extern enum sl_error
sl_hash_table_erase
  (struct sl_context* ctxt,
   struct sl_hash_table* hash_table,
   const void* key,
   size_t* out_nb_erased);

extern enum sl_error
sl_hash_table_find
  (struct sl_context* ctxt,
   struct sl_hash_table* hash_table,
   const void* key,
   void** data);

extern enum sl_error
sl_hash_table_data_count
  (struct sl_context* ctxt,
   struct sl_hash_table* hash_table,
   size_t *nb_data);

extern enum sl_error
sl_hash_table_resize
  (struct sl_context* ctxt,
   struct sl_hash_table* hash_table,
   size_t hint_nb_buckets); 

extern enum sl_error
sl_hash_table_bucket_count
  (struct sl_context* ctxt,
   struct sl_hash_table* hash_table,
   size_t* nb_buckets);

extern enum sl_error
sl_hash_table_used_bucket_count
  (struct sl_context* ctxt,
   struct sl_hash_table* hash_table,
   size_t* nb_used_buckets); 

extern enum sl_error
sl_hash_table_clear
  (struct sl_context* ctxt,
   struct sl_hash_table* hash_table);

#endif /* SL_HASH_TABLE_H */

