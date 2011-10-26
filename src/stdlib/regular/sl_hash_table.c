#include "stdlib/regular/sl_context_c.h"
#include "stdlib/sl_hash_table.h"
#include "sys/sys.h"
#include <assert.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct entry {
  struct entry* next;
  void* data;
};

struct sl_hash_table {
  struct entry** buffer;
  int (*compare)(const void*, const void*);
  const void* (*get_key)(const void*);
  size_t data_size;
  size_t data_alignment;
  size_t key_size;
  size_t nb_buckets;
  size_t nb_used_buckets;
  size_t nb_elements;
};

/*******************************************************************************
 *
 * Fowler/Noll/Vo hash function.
 *
 ******************************************************************************/
static FINLINE uint32_t
fnv32(const void* data, size_t len)
{
  #define FNV32_PRIME (uint32_t)(((uint32_t)1<<24) + ((uint32_t)1<<8) + 0x93)
  #define OFFSET32_BASIS 2166136261u

  const char* octets = data;
  uint32_t hash = OFFSET32_BASIS;
  size_t i;

  for(i=0; i<len; ++i) {
    hash = hash ^ octets[i];
    hash = hash * FNV32_PRIME;
  }
  return hash;

  #undef FNV32_PRIME
  #undef OFFSET32_BASIS
}

static FINLINE uint64_t
fnv64(const void* data, size_t len)
{
  #define FNV64_PRIME (uint64_t)(((uint64_t)1<<40) + ((uint64_t)1<<8) + 0xB3)
  #define OFFSET64_BASIS 14695981039346656037u

  const char* octets = data;
  uint64_t hash = OFFSET64_BASIS;
  size_t i;

  for(i=0; i<len; ++i) {
    hash = hash ^ octets[i];
    hash = hash * FNV64_PRIME;
  }
  return hash;

  #undef FNV64_PRIME
  #undef OFFSET64_BASIS
}

/*******************************************************************************
 *
 * Murmur hash functions.
 *
 ******************************************************************************/
static FINLINE uint32_t
murmur_hash2_32(const void* data, size_t len, uint32_t seed)
{
  #define M 0x5BD1E995
  #define R 24

  uint32_t hash = seed ^ len;
  const char* octets = data;

  while(len >= 4) {
    union {
      char c[4];
      uint32_t i;
    } k = { .c = { octets[0], octets[1], octets[2], octets[3] }};
    k.i *= M;
    k.i ^= k.i >> R;
    k.i *= M;

    hash *= M;
    hash ^= k.i;

    octets += 4;
    len -= 4;
  }

  switch(len) {
    case 3: hash ^= octets[2] << 16;
    case 2: hash ^= octets[1] << 8;
    case 1: hash ^= octets[0];
            hash *= M;
  }

  hash ^= hash >> 13;
  hash *= M;
  hash ^= hash >> 15;

  return hash;

  #undef M
  #undef R
}

static FINLINE uint32_t
murmur_hash2_64(const void* data, size_t len, uint64_t seed)
{
  #define M 0xC6A4A7935BD1E995
  #define R 47

  uint64_t hash = seed ^ (len * M);
  const char* octets = (const char*)data;

  while(len >= 8) {
    union {
      char c[8];
      uint64_t i;
    } k = { 
      .c = { 
        octets[0], octets[1], octets[2], octets[3], 
        octets[4], octets[5], octets[6], octets[7]
      }
    };
    k.i *= M; 
    k.i ^= k.i >> R; 
    k.i *= M; 
    
    hash ^= k.i;
    hash *= M; 

    octets += 8;
    len -= 4;
  }

  switch(len) {
    case 7: hash ^= ((uint64_t)octets[6]) << 48;
    case 6: hash ^= ((uint64_t)octets[5]) << 40;
    case 5: hash ^= ((uint64_t)octets[4]) << 32;
    case 4: hash ^= ((uint64_t)octets[3]) << 24;
    case 3: hash ^= ((uint64_t)octets[2]) << 16;
    case 2: hash ^= ((uint64_t)octets[1]) << 8;
    case 1: hash ^= ((uint64_t)octets[0]);
            hash *= M;
  };

  hash ^= hash >> R;
  hash *= M;
  hash ^= hash >> R;

  return hash;

  #undef M
  #undef R
}

/*******************************************************************************
 *
 * Helper functions
 *
 ******************************************************************************/
static FINLINE size_t
compute_hash(const void* key, size_t key_size, size_t max_hash)
{
  static size_t i = 0;
  static size_t r = 0;
  size_t hash = 0;

  if(max_hash != i) {
    i = max_hash;
    r = rand();
  }

  /* We assume that an uint<32|64>_t can be encoded in a size_t. */
  STATIC_ASSERT
    (UINT32_MAX <= SIZE_MAX && UINT64_MAX <= SIZE_MAX, unexpected_type_size);

  /* We assume that the size is < 2^64-1 and is a power of two. */
  assert(key && max_hash <= UINT64_MAX && SL_IS_POWER_OF_2(max_hash));

  #if 0
  if(max_hash <= UINT32_MAX)
    hash = (size_t)murmur_hash2_32(key, key_size, 0);
  else
    hash = (size_t)murmur_hash2_64(key, key_size, 0);
  #else
  if(max_hash <= UINT32_MAX)
    hash = (size_t)fnv32(key, key_size);
  else
    hash = (size_t)fnv64(key, key_size);
  #endif

  hash = hash & (max_hash - 1); /* Hash % size. */
  return hash;
}

static void
rehash
  (struct entry** dst,
   size_t dst_length,
   struct entry** src,
   size_t src_length,
   const void* (*get_key)(const void*),
   size_t key_size)
{
  size_t i = 0;

  assert(dst && dst_length && (src || !src_length) && get_key && key_size);

  for(i = 0; i < src_length; ++i) {
    struct entry* entry = src[i];
    while(entry) {
      struct entry* next = entry->next;
      const size_t hash = compute_hash
        (get_key(entry->data), key_size, dst_length);
      entry->next = dst[hash];
      dst[hash] = entry;
      entry = next;
    }
  }
}

/*******************************************************************************
 *
 * Implementation of the hash table functions.
 *
 ******************************************************************************/
EXPORT_SYM enum sl_error
sl_create_hash_table
  (struct sl_context* ctxt,
   size_t data_size,
   size_t data_alignment,
   size_t key_size,
   int (*compare)(const void*, const void*),
   const void* (*get_key)(const void*),
   struct sl_hash_table** out_hash_table)
{
  struct sl_hash_table* table = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt
  || !data_size
  || !key_size
  || !compare
  || !get_key
  || !out_hash_table) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  if(!SL_IS_POWER_OF_2(data_alignment)) {
    err = SL_ALIGNMENT_ERROR;
    goto error;
  }

  table = calloc(1, sizeof(struct sl_hash_table));
  if(table == NULL) {
    err = SL_MEMORY_ERROR;
    goto error;
  }

  table->data_size = data_size;
  table->data_alignment = data_alignment;
  table->key_size = key_size;
  table->compare = compare;
  table->get_key = get_key;

exit:
  if(out_hash_table)
    *out_hash_table = table;
  return err;

error:
  if(table)
    free(table);
  goto exit;
}

EXPORT_SYM enum sl_error
sl_free_hash_table
  (struct sl_context* ctxt,
   struct sl_hash_table* table)
{
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !table) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  err = sl_hash_table_clear(ctxt, table);
  if(err != SL_NO_ERROR)
    goto error;

  free(table->buffer);
  free(table);

exit:
  return err;

error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_hash_table_insert
  (struct sl_context* ctxt,
   struct sl_hash_table* table,
   const void* data)
{
  #define HASH_TABLE_BASE_SIZE 32

  struct entry* entry = NULL;
  const void* key = NULL;
  size_t hash = 0;
  size_t previous_nb_used_buckets = 0;
  enum sl_error err = SL_NO_ERROR;
  bool is_inserted = false;

  if(!ctxt || !table || !data) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }
  previous_nb_used_buckets = table->nb_used_buckets;

  if(!IS_ALIGNED(data, table->data_alignment)) {
    err = SL_ALIGNMENT_ERROR;
    goto error;
  }

  if(table->nb_used_buckets >= (2 * table->nb_buckets) / 3) {
    if(table->nb_buckets == 0)
      err = sl_hash_table_resize(ctxt, table, HASH_TABLE_BASE_SIZE);
    else
      err = sl_hash_table_resize(ctxt, table, table->nb_buckets * 2);
    if(err != SL_NO_ERROR)
      goto error;
  }

  entry = malloc(sizeof(struct entry));
  if(entry == NULL) {
    err = SL_MEMORY_ERROR;
    goto error;
  }

  entry->data = memalign(table->data_alignment, table->data_size);
  if(entry->data == NULL) {
    err = SL_MEMORY_ERROR;
    goto error;
  }
  entry->data = memcpy(entry->data, data, table->data_size);

  key = table->get_key(data);
  hash = compute_hash(key, table->key_size, table->nb_buckets);
  entry->next = table->buffer[hash];
  table->nb_used_buckets += (table->buffer[hash] == NULL);
  table->buffer[hash] = entry;
  ++table->nb_elements;
  is_inserted = true;

exit:
  return err;

error:
  if(is_inserted) {
    assert(entry);
    table->buffer[hash] = entry->next;
    table->nb_used_buckets = previous_nb_used_buckets;
    --table->nb_elements;
  }
  if(entry) {
    if(entry->data)
      free(entry->data);
    free(entry);
  }
  goto exit;

  #undef HASH_TABLE_BASE_SIZE
}

EXPORT_SYM enum sl_error
sl_hash_table_erase
  (struct sl_context* ctxt,
   struct sl_hash_table* table,
   const void* key,
   size_t* out_nb_erased)
{
  struct entry* entry = NULL;
  struct entry* previous = NULL;
  size_t hash = 0;
  size_t nb_erased = 0;
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !table || !key || !out_nb_erased) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  hash = compute_hash(key, table->key_size, table->nb_buckets);
  entry = table->buffer[hash];

  while(entry) {
    if(table->compare(table->get_key(entry->data), key) == 0) {
      struct entry* next = entry->next;
      if(previous)
        previous->next = entry->next;
      else
        table->buffer[hash] = entry->next;

      free(entry->data);
      free(entry);

      entry = next;
      --table->nb_elements;
      ++nb_erased;
    } else {
      previous = entry;
      entry = entry->next;
    }
    table->nb_used_buckets -= (table->buffer[hash] == NULL);
  }

exit:
  if(out_nb_erased)
    *out_nb_erased = nb_erased;
  return err;

error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_hash_table_find
  (struct sl_context* ctxt,
   struct sl_hash_table* table,
   const void* key,
   void** out_data)
{
  struct entry* entry = NULL;
  size_t hash = 0;
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !table || !key || !out_data) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  if(table->nb_buckets) {
    hash = compute_hash(key, table->key_size, table->nb_buckets);
    entry = table->buffer[hash];

    while(entry && table->compare(table->get_key(entry->data), key) != 0)
      entry = entry->next;
  }

exit:
  if(out_data)
    *out_data = entry && err == SL_NO_ERROR ? entry->data : NULL;
  return err;

error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_hash_table_data_count
  (struct sl_context* ctxt,
   struct sl_hash_table* table,
   size_t* out_nb_data)
{
  if(!ctxt || !table || !out_nb_data)
    return SL_INVALID_ARGUMENT;

  *out_nb_data = table->nb_elements;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_hash_table_resize
  (struct sl_context* ctxt,
   struct sl_hash_table* table,
   size_t nb_buckets)
{
  struct entry** new_buffer = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !table) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  SL_NEXT_POWER_OF_2(nb_buckets, nb_buckets);

  if(nb_buckets > table->nb_buckets) {
    new_buffer = calloc(nb_buckets, sizeof(struct entry*));
    if(new_buffer == NULL) {
      err = SL_MEMORY_ERROR;
      goto error;
    }
    rehash
      (new_buffer, nb_buckets,
       table->buffer, table->nb_buckets,
       table->get_key, table->key_size);
    free(table->buffer);
    table->buffer = new_buffer;
    table->nb_buckets = nb_buckets;
  }

exit:
  return err;

error:
  if(new_buffer)
    free(new_buffer);
  goto exit;
}

EXPORT_SYM enum sl_error
sl_hash_table_bucket_count
  (struct sl_context* ctxt,
   struct sl_hash_table* table,
   size_t* nb_buckets)
{
  if(!ctxt || !table || !nb_buckets)
    return SL_INVALID_ARGUMENT;

  *nb_buckets = table->nb_buckets;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_hash_table_used_bucket_count
  (struct sl_context* ctxt,
   struct sl_hash_table* table,
   size_t* nb_used_buckets)
{
  if(!ctxt || !table || !nb_used_buckets)
    return SL_INVALID_ARGUMENT;

  *nb_used_buckets = table->nb_used_buckets;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_hash_table_clear
  (struct sl_context* ctxt,
   struct sl_hash_table* table)
{
  size_t i = 0;

  if(!ctxt || !table)
    return SL_INVALID_ARGUMENT;

  for(i = 0; i < table->nb_buckets; ++i) {
    struct entry* entry = table->buffer[i];
    while(entry) {
      struct entry* next_entry = entry->next;
      free(entry->data);
      free(entry);
      entry = next_entry;
    }
    table->buffer[i] = NULL;
  }
  table->nb_elements = 0;
  table->nb_used_buckets = 0;

  return SL_NO_ERROR;
}

