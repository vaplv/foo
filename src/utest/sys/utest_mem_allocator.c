#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "utest/utest.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
regular_test(struct mem_allocator* allocator)
{
  char dump[BUFSIZ];
  void* p = NULL;
  void* q[3] = {NULL, NULL, NULL};
  size_t i = 0;

  p = MEM_ALIGNED_ALLOC_I(allocator, 1024, ALIGNOF(char));
  NCHECK(p, NULL);
  CHECK(IS_ALIGNED((uintptr_t)p, ALIGNOF(char)), true);
  MEM_FREE_I(allocator, p);

  q[0] = MEM_ALIGNED_ALLOC_I(allocator, 10, 8);
  q[1] = MEM_CALLOC_I(allocator, 1, 58);
  q[2] = MEM_ALLOC_I(allocator, 78);
  NCHECK(q[0], NULL);
  NCHECK(q[1], NULL);
  NCHECK(q[2], NULL);
  CHECK(IS_ALIGNED((uintptr_t)q[0], 8), true);

  p = MEM_CALLOC_I(allocator, 2, 2);
  NCHECK(p, NULL);
  for(i = 0; i < 4; ++i)
    CHECK(((char*)p)[i], 0);
  for(i = 0; i < 4; ++i)
    ((char*)p)[i] = (char)i;

  MEM_DUMP_I(allocator, dump, BUFSIZ, NULL);
  printf("%s\n", dump);

  MEM_FREE_I(allocator, q[1]);

  p = MEM_REALLOC_I(allocator, p, 8);
  for(i = 0; i < 4; ++i)
    CHECK(((char*)p)[i], (char)i);
  for(i = 4; i < 8; ++i)
    ((char*)p)[i] = (char)i;

  MEM_FREE_I(allocator, q[2]);

  p = MEM_REALLOC_I(allocator, p, 5);
  for(i = 0; i < 5; ++i)
    CHECK(((char*)p)[i], (char)i);

  MEM_FREE_I(allocator, p);

  p = NULL;
  p = MEM_REALLOC_I(allocator, NULL, 16);
  NCHECK(p, NULL);
  p = MEM_REALLOC_I(allocator, p, 0);

  MEM_FREE_I(allocator, q[0]);

  CHECK(MEM_ALIGNED_ALLOC_I(allocator, 1024, 0), NULL);
  CHECK(MEM_ALIGNED_ALLOC_I(allocator, 1024, 3), NULL);
  CHECK(MEM_ALLOCATED_SIZE_I(allocator), 0);
}

int
main(int argc UNUSED, char** argv UNUSED)
{
  struct mem_allocator allocator;
  struct mem_sys_info mem_info;

  printf("Default allocator:\n");
  regular_test(&mem_default_allocator);

  printf("\nProxy allocator\n");
  mem_init_proxy_allocator("utest", &allocator, &mem_default_allocator);
  regular_test(&allocator);
  mem_shutdown_proxy_allocator(&allocator);

  mem_sys_info(&mem_info);
  printf
    ("\nHeap summary:\n"
     "  total heap size: %zu bytes\n"
     "  in use size: %zu bytes\n",
     mem_info.total_size,
     mem_info.used_size);

  return 0;
}
