#ifndef EDIT_CONTEXT_H
#define EDIT_CONTEXT_H

#include "app/editor/edit.h"
#include "app/editor/edit_error.h"
#include <stdbool.h>

#ifndef NDEBUG
  #include <assert.h>
  #define EDIT(func) assert(EDIT_NO_ERROR == edit_##func)
#else
  #define EDIT(func) edit_##func
#endif /* NDEBUG */

struct app;
struct edit_context;
struct mem_allocator;

EDIT_API enum edit_error
edit_create_context
  (struct app* app,
   struct mem_allocator* allocator,
   struct edit_context** out_ctxt);

EDIT_API enum edit_error
edit_context_ref_get
  (struct edit_context* ctxt);

EDIT_API enum edit_error
edit_context_ref_put
  (struct edit_context* ctxt);

EDIT_API enum edit_error
edit_run
  (struct edit_context* ctxt);

EDIT_API enum edit_error
edit_enable_inputs
  (struct edit_context* ctxt,
   bool is_enable);

#endif /* EDIT_CONTEXT_H */

