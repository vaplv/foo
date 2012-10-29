#ifndef EDIT_CONTEXT_C_H
#define EDIT_CONTEXT_C_H

#include "app/editor/regular/edit_cvars.h"
#include "app/editor/regular/edit_inputs.h"
#include "maths/simd/aosf44.h"
#include "sys/ref_count.h"
#include <stdbool.h>

/* Helper macros. */
#define EDIT_CMD_ARGVAL(argv, i) EDIT_CMD_ARGVAL_N(argv, i, 0)
#define EDIT_CMD_ARGVAL_N(argv, i, n) (argv)[(i)]->value_list[(n)]

struct app;
struct app_cvar;
struct edit_model_instance_selection;
struct mem_allocator;

ALIGN(16) struct edit_context {
  struct ref ref;
  struct app* app;
  struct mem_allocator* allocator;
  struct edit_model_instance_selection* instance_selection;
  struct edit_inputs* inputs;
  struct edit_cvars cvars; 
  bool process_inputs; /* Define if the input events are intercepted  */
};


#endif /* EDIT_CONTEXT_C_H */

