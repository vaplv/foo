#ifndef EDIT_CONTEXT_C_H
#define EDIT_CONTEXT_C_H

#include "sys/ref_count.h"

/* Helper macro. */
#define EDIT_CMD_ARGVAL(argv, i) (argv)[i]->value_list[0]

struct app;
struct mem_allocator;

struct edit_context {
  struct ref ref;
  struct app* app;
  struct mem_allocator* allocator;
};

#endif /* EDIT_CONTEXT_C_H */

