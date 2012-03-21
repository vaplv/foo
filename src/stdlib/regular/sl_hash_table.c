#include "stdlib/regular/sl_c.h"
#include "stdlib/sl_hash_table.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct entry {
  struct entry* next;
  struct {
    union data {
      void* ptr;
      char value[sizeof(void*)];
    } key, data;
  } pair;
};

struct sl_hash_table {
  struct entry** buffer;
  size_t (*hash_fcn)(const void*);
  bool (*eq_key)(const void*, const void*);
  struct mem_allocator* allocator;
  size_t data_size;
  size_t data_alignment;
  size_t key_size;
  size_t key_alignment;
  size_t nb_buckets;
  size_t nb_used_buckets;
  size_t nb_elements;
  /* Define whether the key/data of an entry is stored in allocated memory
   * space or not. */
  bool alloc_key;
  bool alloc_data;
};

/* We assume that an uint<32|64>_t can be encoded in a size_t. */
STATIC_ASSERT
  (UINT32_MAX <= SIZE_MAX && UINT64_MAX <= SIZE_MAX, unexpected_type_size);

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
  assert(!len || data);

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
  assert(!len || data);

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
  assert(!len || data);

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
  assert(!len || data);

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
static FINLINE void*
get(union data* data, bool is_data_allocated) 
{
  assert(data);
  return is_data_allocated ? data->ptr : data->value;
}

static FINLINE size_t
compute_bucket(size_t (*hash_fcn)(const void*), const void* key, size_t max_hash)
{
  size_t hash = 0;
  /* We assume that the size is a power of two. */
  assert(hash_fcn && key && SL_IS_POWER_OF_2(max_hash));

  hash = hash_fcn(key);
  hash = hash & (max_hash - 1); /* Hash % size. */
  return hash;
}

static void
rehash
  (struct entry** dst,
   size_t dst_length,
   struct entry** src,
   size_t src_length,
   size_t (*hash_fcn)(const void*),
   bool is_key_allocated)
{
  size_t i = 0;

  assert(dst && dst_length && (src || !src_length) && hash_fcn);

  for(i = 0; i < src_length; ++i) {
    struct entry* entry = src[i];
    while(entry) {
      struct entry* next = entry->next;
      const size_t bucket = compute_bucket
        (hash_fcn, get(&entry->pair.key, is_key_allocated), dst_length);
      entry->next = dst[bucket];
      dst[bucket] = entry;
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
  (size_t key_size,
   size_t key_alignment,
   size_t data_size,
   size_t data_alignment,
   size_t (*hash_fcn)(const void*),
   bool (*eq_key)(const void*, const void*),
   struct mem_allocator* specific_allocator,
   struct sl_hash_table** out_hash_table)
{
  struct mem_allocator* allocator = NULL;
  struct sl_hash_table* table = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!data_size
  || !key_size
  || !hash_fcn
  || !eq_key
  || !out_hash_table) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }
  if(!SL_IS_POWER_OF_2(data_alignment) || !SL_IS_POWER_OF_2(key_alignment)) {
    err = SL_ALIGNMENT_ERROR;
    goto error;
  }
  allocator = specific_allocator ? specific_allocator : &mem_default_allocator;
  table = MEM_CALLOC(allocator, 1, sizeof(struct sl_hash_table));
  if(table == NULL) {
    err = SL_MEMORY_ERROR;
    goto error;
  }

  table->data_size = data_size;
  table->data_alignment = data_alignment;
  table->key_size = key_size;
  table->key_alignment = key_alignment;
  table->hash_fcn = hash_fcn;
  table->eq_key = eq_key;
  table->allocator = allocator;
  #if 1
  table->alloc_key = key_alignment > ALIGNOF(void*) || key_size > sizeof(void*);
  table->alloc_data = data_alignment > ALIGNOF(void*) || data_size > sizeof(void*);
  #else
  table->alloc_key = table->alloc_data = true;
  #endif
exit:
  if(out_hash_table)
    *out_hash_table = table;
  return err;

error:
  if(table)
    MEM_FREE(allocator, table);
  goto exit;
}

EXPORT_SYM enum sl_error
sl_free_hash_table
  (struct sl_hash_table* table)
{
  struct mem_allocator* allocator = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!table) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }
  err = sl_hash_table_clear(table);
  if(err != SL_NO_ERROR)
    goto error;

  allocator = table->allocator;
  MEM_FREE(allocator, table->buffer);
  MEM_FREE(allocator, table);

exit:
  return err;

error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_hash_table_insert
  (struct sl_hash_table* table,
   const void* key,
   const void* data)
{
  #define HASH_TABLE_BASE_SIZE 32

  struct entry* entry = NULL;
  size_t bucket = 0;
  size_t previous_nb_used_buckets = 0;
  enum sl_error err = SL_NO_ERROR;
  bool is_inserted = false;

  if(!table || !key || !data) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }
  previous_nb_used_buckets = table->nb_used_buckets;

  if(!IS_ALIGNED(data, table->data_alignment)
  || !IS_ALIGNED(key, table->key_alignment)) {
    err = SL_ALIGNMENT_ERROR;
    goto error;
  }
  if(table->nb_used_buckets >= (2 * table->nb_buckets) / 3) {
    if(table->nb_buckets == 0)
      err = sl_hash_table_resize(table, HASH_TABLE_BASE_SIZE);
    else
      err = sl_hash_table_resize(table, table->nb_buckets * 2);
    if(err != SL_NO_ERROR)
      goto error;
  }
  entry = MEM_ALLOC(table->allocator, sizeof(struct entry));
  if(entry == NULL) {
    err = SL_MEMORY_ERROR;
    goto error;
  }
  if(table->alloc_data == false) {
    assert(IS_ALIGNED(entry->pair.data.value, table->data_alignment));
    memcpy(entry->pair.data.value, data, table->data_size);
  } else {
    entry->pair.data.ptr = MEM_ALIGNED_ALLOC
      (table->allocator, table->data_size, table->data_alignment);
    if(entry->pair.data.ptr == NULL) {
      err = SL_MEMORY_ERROR;
      goto error;
    }
    memcpy(entry->pair.data.ptr, data, table->data_size);
  }
  if(table->alloc_key == false) {
    assert(IS_ALIGNED(entry->pair.key.value, table->key_alignment));
    memcpy(entry->pair.key.value, key, table->key_size);
  } else {
    entry->pair.key.ptr = MEM_ALIGNED_ALLOC
      (table->allocator, table->key_size, table->key_alignment);
    if(entry->pair.key.ptr == NULL) {
      err = SL_MEMORY_ERROR;
      goto error;
    }
    memcpy(entry->pair.key.ptr, key, table->key_size);
  }

  bucket = compute_bucket(table->hash_fcn, key, table->nb_buckets);
  entry->next = table->buffer[bucket];
  table->nb_used_buckets += (table->buffer[bucket] == NULL);
  table->buffer[bucket] = entry;
  ++table->nb_elements;
  is_inserted = true;

exit:
  return err;
error:
  if(is_inserted) {
    assert(entry);
    table->buffer[bucket] = entry->next;
    table->nb_used_buckets = previous_nb_used_buckets;
    --table->nb_elements;
  }
  if(entry) {
    if(table->alloc_data && entry->pair.data.ptr)
      MEM_FREE(table->allocator, entry->pair.data.ptr);
    if(table->alloc_key && entry->pair.key.ptr)
      MEM_FREE(table->allocator, entry->pair.key.ptr);
    MEM_FREE(table->allocator, entry);
  }
  goto exit;

  #undef HASH_TABLE_BASE_SIZE
}

EXPORT_SYM enum sl_error
sl_hash_table_erase
  (struct sl_hash_table* table,
   const void* key,
   size_t* out_nb_erased)
{
  struct entry* entry = NULL;
  struct entry* previous = NULL;
  size_t bucket = 0;
  size_t nb_erased = 0;
  enum sl_error err = SL_NO_ERROR;

  if(!table || !key) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  bucket = compute_bucket(table->hash_fcn, key, table->nb_buckets);
  entry = table->buffer[bucket];

  while(entry) {
    if(table->eq_key(get(&entry->pair.key, table->alloc_key), key) == true) {
      struct entry* next = entry->next;
      if(previous)
        previous->next = entry->next;
      else
        table->buffer[bucket] = entry->next;

      if(table->alloc_data)
        MEM_FREE(table->allocator, entry->pair.data.ptr);
      if(table->alloc_key)
        MEM_FREE(table->allocator, entry->pair.key.ptr);
      MEM_FREE(table->allocator, entry);

      entry = next;
      --table->nb_elements;
      ++nb_erased;
    } else {
      previous = entry;
      entry = entry->next;
    }
    table->nb_used_buckets -= (table->buffer[bucket] == NULL);
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
  (struct sl_hash_table* table,
   const void* key,
   void** out_data)
{
  struct sl_pair pair;
  enum sl_error err = SL_NO_ERROR;

  if(!out_data) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }
  err = sl_hash_table_find_pair(table, key, &pair);
  if(err != SL_NO_ERROR)
    goto error;
  *out_data = pair.data;
exit:
  return err;
error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_hash_table_find_pair
  (struct sl_hash_table* table,
   const void* key,
   struct sl_pair* pair)
{
  struct entry* entry = NULL;
  size_t bucket = 0;
  enum sl_error err = SL_NO_ERROR;
  pair->key = pair->data = NULL;

  if(!table || !key || !pair) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }
  if(table->nb_buckets) {
    bucket = compute_bucket(table->hash_fcn, key, table->nb_buckets);
    entry = table->buffer[bucket];

    while(entry != NULL
       && table->eq_key(get(&entry->pair.key, table->alloc_key), key) != true)
      entry = entry->next;
  }
  if(entry && err == SL_NO_ERROR) {
    pair->key = get(&entry->pair.key, table->alloc_key);
    pair->data = get(&entry->pair.data, table->alloc_data);
  }
exit:
  return err;
error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_hash_table_data_count
  (struct sl_hash_table* table,
   size_t* out_nb_data)
{
  if(!table || !out_nb_data)
    return SL_INVALID_ARGUMENT;

  *out_nb_data = table->nb_elements;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_hash_table_resize
  (struct sl_hash_table* table,
   size_t nb_buckets)
{
  struct entry** new_buffer = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!table) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  SL_NEXT_POWER_OF_2(nb_buckets, nb_buckets);

  if(nb_buckets > table->nb_buckets) {
    new_buffer = MEM_CALLOC
      (table->allocator, nb_buckets, sizeof(struct entry*));
    if(new_buffer == NULL) {
      err = SL_MEMORY_ERROR;
      goto error;
    }
    rehash
      (new_buffer, nb_buckets,
       table->buffer, table->nb_buckets,
       table->hash_fcn, table->alloc_key);
    MEM_FREE(table->allocator, table->buffer);
    table->buffer = new_buffer;
    table->nb_buckets = nb_buckets;
  }

exit:
  return err;

error:
  if(new_buffer)
    MEM_FREE(table->allocator, new_buffer);
  goto exit;
}

EXPORT_SYM enum sl_error
sl_hash_table_bucket_count
  (const struct sl_hash_table* table,
   size_t* nb_buckets)
{
  if(!table || !nb_buckets)
    return SL_INVALID_ARGUMENT;

  *nb_buckets = table->nb_buckets;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_hash_table_used_bucket_count
  (const struct sl_hash_table* table,
   size_t* nb_used_buckets)
{
  if(!table || !nb_used_buckets)
    return SL_INVALID_ARGUMENT;

  *nb_used_buckets = table->nb_used_buckets;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_hash_table_clear
  (struct sl_hash_table* table)
{
  size_t i = 0;

  if(!table)
    return SL_INVALID_ARGUMENT;

  for(i = 0; i < table->nb_buckets; ++i) {
    struct entry* entry = table->buffer[i];
    while(entry) {
      struct entry* next_entry = entry->next;
      if(table->alloc_data)
        MEM_FREE(table->allocator, entry->pair.data.ptr);
      if(table->alloc_key)
        MEM_FREE(table->allocator, entry->pair.key.ptr);
      MEM_FREE(table->allocator, entry);
      entry = next_entry;
    }
    table->buffer[i] = NULL;
  }
  table->nb_elements = 0;
  table->nb_used_buckets = 0;

  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_hash_table_begin
  (struct sl_hash_table* table,
   struct sl_hash_table_it* it,
   bool* is_end_reached)
{
  size_t i = 0;

  if(!table || !it || !is_end_reached)
    return SL_INVALID_ARGUMENT;

  for(i = 0; i < table->nb_buckets; ++i) {
    if(table->buffer[i] != NULL) {
      it->bucket = i;
      it->entry = table->buffer[i];
      it->pair.key = get(&it->entry->pair.key, table->alloc_key);
      it->pair.data = get(&it->entry->pair.data, table->alloc_data);
      it->hash_table = table;
      break;
    }
  }
  *is_end_reached = (i >= table->nb_buckets);
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_hash_table_it_next(struct sl_hash_table_it* it, bool* is_end_reached)
{
  struct entry* entry = NULL;

  if(!it
  || !it->hash_table
  || !is_end_reached)
    return SL_INVALID_ARGUMENT;

  entry = it->entry;
  if(entry->next) {
    it->entry = entry->next;
    it->pair.key = get(&it->entry->pair.key, it->hash_table->alloc_key);
    it->pair.data = get(&it->entry->pair.data, it->hash_table->alloc_data);
  } else {
    struct sl_hash_table* table = it->hash_table;
    size_t i = 0;
    for(i = it->bucket + 1; i < table->nb_buckets;  ++i) {
      if(it->hash_table->buffer[i] != NULL) {
        it->bucket = i;
        it->entry = table->buffer[i];
        it->pair.key = get(&it->entry->pair.key, it->hash_table->alloc_key);
        it->pair.data = get(&it->entry->pair.data, it->hash_table->alloc_data);
        it->hash_table = table;
        break;
      }
    }
    *is_end_reached = (i >= table->nb_buckets);
  }
  return SL_NO_ERROR;
}

/*******************************************************************************
 *
 * Generic hash functions.
 *
 ******************************************************************************/
EXPORT_SYM size_t
sl_hash(const void* data, size_t len)
{
  size_t hash = 0;
  #define FNV32

  #if defined(FNV32)
  hash = (size_t)fnv32(data, len);
  #elif defined(FNV64)
  hash = (size_t)fnv64(data, len);
  #elif defined(MURMUR32)
  hash = (size_t)murmur_hash2_32(data, len, 0);
  #elif defined(MURMUR64)
  hash = (size_t)murmur_hash2_64(data, len, 0);
  #else
  #error Unreachable_code
  #endif

  #undef FNV32
  return hash;
}

