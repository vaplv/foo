#ifndef EDIT_CONTEXT_C_H
#define EDIT_CONTEXT_C_H

#include "sys/ref_count.h"

/* Helper macros. */
#define EDIT_CMD_ARGVAL(argv, i) EDIT_CMD_ARGVAL_N(argv, i, 0)
#define EDIT_CMD_ARGVAL_N(argv, i, n) (argv)[(i)]->value_list[(n)]

struct app;
struct mem_allocator;
struct sl_hash_table;

struct edit_context {
  struct ref ref;
  struct app* app;
  struct mem_allocator* allocator;
  struct sl_hash_table* selected_model_instance_htbl;
};

#endif /* EDIT_CONTEXT_C_H */

