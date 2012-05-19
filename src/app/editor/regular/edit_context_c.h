#ifndef EDIT_CONTEXT_C_H
#define EDIT_CONTEXT_C_H

#include "sys/ref_count.h"

/* Helper macros. */
#define EDIT_CMD_ARGVAL(argv, i) EDIT_CMD_ARGVAL_N(argv, i, 0)
#define EDIT_CMD_ARGVAL_N(argv, i, n) (argv)[(i)]->value_list[(n)]

struct app;
struct app_cvar;
struct edit_model_instance_selection;
struct mem_allocator;

struct edit_context {
  struct ref ref;
  struct app* app;
  struct mem_allocator* allocator;
  struct edit_model_instance_selection* instance_selection;
  struct cvars {
    const struct app_cvar* project_path;
    const struct app_cvar* show_selection;
  } cvars;
};

#endif /* EDIT_CONTEXT_C_H */

