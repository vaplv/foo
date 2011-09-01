#ifndef SL_HASH_TABLE_H
#define SL_HASH_TABLE_H

#include "stdlib/sl_error.h"
#include <stddef.h>

/* TODO */
#error "The hash table functions are not implemented yet!"

struct sl_context;
struct sl_hash_table;

extern enum sl_error;
sl_create_hash_table
  (struct sl_context* ctxt,
   size_t data_size,
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
   const void* elmt);

extern enum sl_error
sl_hash_table_erase
  (struct sl_context* ctxt,
   struct sl_hash_table* hash_table,
   const void* key);

extern enum sl_error
sl_hash_table_find
  (struct sl_context* ctxt,
   struct sl_hash_table* hash_table,
   const void* key,
   void** elmt);

extern enum sl_error
sl_hash_table_resize
  (struct sl_context* ctxt,
   struct sl_hash_table* hash_table,
   size_t bucket_count);

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

