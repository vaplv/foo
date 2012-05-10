#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "sys/math.h"
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

EXPORT_SYM void
mem_sys_info(struct mem_sys_info* mem_info)
{
  const struct mallinfo info = mallinfo();

  assert(mem_info != NULL);
  mem_info->total_size = info.arena;
  mem_info->used_size = info.usmblks + info.uordblks;
}

/*******************************************************************************
 *
 * Default allocator functions.
 *
 ******************************************************************************/
struct alloc_counter {
  size_t nb_allocs;
  size_t allocated_size;
};

static void*
default_alloc
  (void* data UNUSED,
   size_t size,
   const char* filename UNUSED,
   unsigned int fileline UNUSED)
{
  void* mem = NULL;

  if(size) {
    mem = malloc(size);
    #ifndef NDEBUG
    assert(data);
    if(mem) {
      struct alloc_counter* alloc_counter = data;
      const size_t allocated_size =
        alloc_counter->allocated_size + malloc_usable_size(mem);

      if( (allocated_size < alloc_counter->allocated_size) /* Overflow. */
        | (alloc_counter->nb_allocs == SIZE_MAX)) {
        free(mem);
        mem = NULL;
      } else {
        alloc_counter->allocated_size = allocated_size;
        ++alloc_counter->nb_allocs;
      }
    }
    #endif
  }
  return mem;
}

static void
default_free(void* data UNUSED, void* mem)
{
  if(mem) {
    #ifndef NDEBUG
    struct alloc_counter* alloc_counter = data;
    size_t size_to_free = malloc_usable_size(mem);
    assert
      ( (data != NULL)
      & (alloc_counter->nb_allocs != 0)
      & (alloc_counter->allocated_size >= size_to_free));

    alloc_counter->allocated_size -= size_to_free;
    --alloc_counter->nb_allocs;
    #endif
    free(mem);
  }
}

static void*
default_calloc
  (void* data,
   size_t nbelmts,
   size_t size,
   const char* filename,
   unsigned int fileline)
{
  void* mem = NULL;
  const size_t alloc_size = nbelmts * size;

  mem = default_alloc(data, alloc_size, filename, fileline);
  if(mem)
    memset(mem, 0, alloc_size);
  return mem;
}

static void*
default_realloc
  (void* data UNUSED,
   void* mem,
   size_t size,
   const char* filename UNUSED,
   unsigned int fileline UNUSED)
{
  void* new_mem = NULL;

  #ifdef NDEBUG
  new_mem = realloc(mem, size);
  #else
  assert(data);
  if(!mem) {
    new_mem = default_alloc(data, size, filename, fileline);
  } else {
    if(size == 0) {
      default_free(data, mem);
    } else {
      struct alloc_counter* alloc_counter = data;
      size_t old_size = malloc_usable_size(mem);
      size_t new_allocated_size = 0;

      assert(alloc_counter->allocated_size >= old_size);
      alloc_counter->allocated_size -= old_size;
      new_mem = realloc(mem, size);
      new_allocated_size =
        alloc_counter->allocated_size + malloc_usable_size(new_mem);

      /* Check overflow and new_mem != NULL. */
      if(new_allocated_size <= alloc_counter->allocated_size) {
        if(new_mem) {
          free(new_mem);
          new_mem = NULL;
        }
        assert(alloc_counter->nb_allocs);
        --alloc_counter->nb_allocs;
      } else {
        alloc_counter->allocated_size += malloc_usable_size(new_mem);
      }
    }
  }
  #endif
  return new_mem;
}

static void*
default_aligned_alloc
  (void* data UNUSED,
   size_t size,
   size_t alignment,
   const char* filename UNUSED,
   unsigned int fileline UNUSED)
{
  void* mem = NULL;

  if(size && IS_POWER_OF_2(alignment)) {
    mem = memalign(alignment, size);
    #ifndef NDEBUG
    assert(data);
    if(mem) {
      struct alloc_counter* alloc_counter = data;
      const size_t allocated_size =
        alloc_counter->allocated_size + malloc_usable_size(mem);

      if( (allocated_size < alloc_counter->allocated_size) /* Overflow */
        | (alloc_counter->nb_allocs == SIZE_MAX)) {
        free(mem);
        mem = NULL;
      } else {
        alloc_counter->allocated_size = allocated_size;
        ++alloc_counter->nb_allocs;
      }
    }
    #endif
  }
  return mem;
}

static size_t
default_allocated_size(const void* data UNUSED)
{
  #ifdef NDEBUG
  return 0;
  #else
  const struct alloc_counter* alloc_counter = data;
  assert(alloc_counter != NULL);
  return alloc_counter->allocated_size;
  #endif
}

static size_t
default_dump
  (const void* data UNUSED,
   char* dump,
   size_t max_dump_len)
{
  #ifdef NDEBUG
  if(dump && max_dump_len)
    dump[0] = '\0';
  return 0;
  #else
  const struct alloc_counter* alloc_counter = NULL;
  size_t dump_len = 0;
  int len = 0;

  assert(data && (!max_dump_len || dump));
  alloc_counter = data;

  len = snprintf
    (dump,
     max_dump_len,
     "%zu bytes allocated in %zu allocations.",
     alloc_counter->allocated_size,
     alloc_counter->nb_allocs);
  assert(len >= 0);
  dump_len = (size_t)len;

  if((size_t)len >= (max_dump_len - 1)) /* -1 <=> null char. */
    dump[max_dump_len] = '\0';

  return dump_len;
  #endif
}

/*******************************************************************************
 *
 * Proxy allocator functions.
 *
 ******************************************************************************/
#define PROXY_DEFAULT_ALIGNMENT 8

struct proxy_data {
  const char* name;
  struct mem_allocator* allocator;
  struct mem_node* node_list;
};

struct mem_node {
  struct mem_node* next;
  struct mem_node* prev;
  size_t size;
  const char* filename;
  unsigned int fileline;
  char reserved[2];
  char mem[];
};

static void*
proxy_aligned_alloc
  (void* data,
   size_t size,
   size_t align,
   const char* filename,
   unsigned int fileline)
{
  struct proxy_data* proxy_data = NULL;
  char* mem = NULL;
  size_t node_header_size = 0;
  size_t node_size = 0;
  struct mem_node* node = NULL;

  assert(data);
  proxy_data = data;

  if((IS_POWER_OF_2(align) == 0) || align > 32768)
    return NULL;
  align = align < PROXY_DEFAULT_ALIGNMENT ? PROXY_DEFAULT_ALIGNMENT : align;

  node_header_size = ALIGN_SIZE(sizeof(struct mem_node), align);
  node_size = node_header_size + size;
  node = MEM_ALIGNED_ALLOC(proxy_data->allocator, node_size, align);
  if(!node)
    return NULL;

  mem = (char*)((uintptr_t)node + (uintptr_t)node_header_size);
  mem[-1] = align & 0xFF;
  mem[-2] = (align >> 8) & 0xFF;
  node->next = proxy_data->node_list;
  node->prev = NULL;
  node->filename = filename;
  node->fileline = fileline;
  node->size = size;
  if(proxy_data->node_list)
    proxy_data->node_list->prev = node;
  proxy_data->node_list = node;
  return mem;
}

static void*
proxy_alloc
  (void* data,
   size_t size,
   const char* filename,
   unsigned int fileline)
{
  return proxy_aligned_alloc
    (data, size, PROXY_DEFAULT_ALIGNMENT, filename, fileline);
}

static void*
proxy_calloc
  (void* data,
   size_t nbelmts,
   size_t size,
   const char* filename,
   unsigned int fileline)
{
  size_t allocation_size = nbelmts * size;
  void* mem = proxy_aligned_alloc
    (data, allocation_size, PROXY_DEFAULT_ALIGNMENT, filename, fileline);
  if(mem)
    mem = memset(mem, 0, allocation_size);
  return mem;
}

static void
proxy_free(void* data, void* mem)
{
  if(mem) {
    struct proxy_data* proxy_data = NULL;
    struct mem_node* node = NULL;
    uintptr_t alignment = 0;

    assert(data);
    proxy_data = data;

    alignment = ((char*)mem)[-1] | (((char*)mem)[-2] << 8);
    node =
      (void*)((uintptr_t)mem - ALIGN_SIZE(sizeof(struct mem_node), alignment));

    if(node->prev) {
      node->prev->next = node->next;
    }
    if(node->next) {
      node->next->prev = node->prev;
    }
    if(node->prev == NULL) {
      proxy_data->node_list = node->next;
    }
    MEM_FREE(proxy_data->allocator, node);
  }
}

static void*
proxy_realloc
  (void* data,
   void* mem,
   size_t size,
   const char* filename,
   unsigned int fileline)
{
  if(size == 0) {
    proxy_free(data, mem);
    return NULL;
  } else if(mem == NULL) {
    return proxy_aligned_alloc
      (data, size, PROXY_DEFAULT_ALIGNMENT, filename, fileline);
  } else {
    struct mem_node* node = NULL;
    uintptr_t node_header_size = 0;
    uintptr_t alignment = 0;

    alignment = ((char*)mem)[-1] | (((char*)mem)[-2] << 8);
    node_header_size = ALIGN_SIZE(sizeof(struct mem_node), alignment);
    node = (void*)((uintptr_t)mem - node_header_size);

    if(node->size == size) {
      return mem;
    } else {
      void* dst = proxy_aligned_alloc
        (data, size, alignment, filename, fileline);
      if(!dst) {
        proxy_free(data, mem);
        return NULL;
      } else {
        dst = memcpy(dst, mem, size < node->size ? size : node->size);
        proxy_free(data, mem);
        return dst;
      }
    }
  }
}

static size_t
proxy_allocated_size(const void* data)
{
  const struct proxy_data* proxy_data = NULL;
  struct mem_node* node = NULL;
  size_t allocated_size = 0;

  assert(data);
  proxy_data = data;
  for(node = proxy_data->node_list; node != NULL; node = node->next)
  {
    allocated_size += malloc_usable_size(node);
  }
  return allocated_size;
}

static size_t
proxy_dump
  (const void* data,
   char* dump,
   size_t max_dump_len)
{
  const struct proxy_data* proxy_data = NULL;
  struct mem_node* node = NULL;
  size_t dump_len = 0;
  size_t avaible_dump_space = max_dump_len ? max_dump_len - 1 /*NULL char*/ : 0;

  assert(data && (!max_dump_len || dump));
  proxy_data = data;

  for(node = proxy_data->node_list; node != NULL; node = node->next) {
    if(dump) {
      const int len = snprintf
        (dump,
         avaible_dump_space,
         "%s: %zu bytes allocated at %s:%u%s",
         proxy_data->name,
         malloc_usable_size(node),
         node->filename ? node->filename : "none",
         node->fileline,
         node->next ? ".\n" : ".");
      assert(len >= 0);
      dump_len += len;

      if((size_t)len < avaible_dump_space) {
        dump += len;
        avaible_dump_space -= len;
      } else if(dump) {
        dump[avaible_dump_space] = '\0';
        avaible_dump_space = 0;
        dump = NULL;
      }
    }
  }
  return dump_len;
}

#undef PROXY_DEFAULT_ALIGNMENT

/*******************************************************************************
 *
 * Default allocator.
 *
 ******************************************************************************/
static struct alloc_counter default_alloc_counter = {0, 0};

EXPORT_SYM struct mem_allocator mem_default_allocator = {
  .alloc = default_alloc,
  .calloc = default_calloc,
  .realloc = default_realloc,
  .aligned_alloc = default_aligned_alloc,
  .free = default_free,
  .allocated_size = default_allocated_size,
  .dump = default_dump,
  .data = (void*) &default_alloc_counter
};

/*******************************************************************************
 *
 * Proxy allocator.
 *
 ******************************************************************************/
EXPORT_SYM void
mem_init_proxy_allocator
  (const char* name,
   struct mem_allocator* proxy_allocator,
   struct mem_allocator* allocator)
{
  struct proxy_data* proxy_data = NULL;

  if((!allocator) | (!proxy_allocator))
    goto error;

  proxy_data = MEM_CALLOC(allocator, 1, sizeof(struct proxy_data));
  if(!proxy_data)
    goto error;
  proxy_data->name = name;
  proxy_data->allocator = allocator;
  proxy_data->node_list = NULL;

  proxy_allocator->alloc = proxy_alloc;
  proxy_allocator->calloc = proxy_calloc;
  proxy_allocator->realloc = proxy_realloc;
  proxy_allocator->aligned_alloc = proxy_aligned_alloc;
  proxy_allocator->free = proxy_free;
  proxy_allocator->allocated_size = proxy_allocated_size;
  proxy_allocator->dump = proxy_dump;
  proxy_allocator->data = (void*)proxy_data;

exit:
  return;
error:
  if(proxy_allocator) {
    assert(proxy_data == NULL);
    memset(proxy_allocator, 0, sizeof(struct mem_allocator));
  }
  goto exit;
}

EXPORT_SYM void
mem_shutdown_proxy_allocator(struct mem_allocator* proxy)
{
  struct proxy_data* proxy_data = NULL;
  struct mem_allocator* allocator = NULL;

  assert(proxy);
  proxy_data = proxy->data;
  assert(proxy_data->node_list == NULL);
  allocator = proxy_data->allocator;
  MEM_FREE(allocator, proxy_data);
  memset(proxy, 0, sizeof(struct mem_allocator));
}

