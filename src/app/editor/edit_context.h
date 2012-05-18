#ifndef EDIT_CONTEXT_H
#define EDIT_CONTEXT_H

#include "app/editor/edit_error.h"

#ifndef NDEBUG
  #include <assert.h>
  #define EDIT(func) assert(EDIT_NO_ERROR == edit_##func)
#else
  #define EDIT(func) edit_##func
#endif /* NDEBUG */

struct app;
struct edit_context;
struct mem_allocator;

extern enum edit_error
edit_create_context
  (struct app* app,
   struct mem_allocator* allocator,
   struct edit_context** out_ctxt);

extern enum edit_error
edit_context_ref_get
  (struct edit_context* ctxt);

extern enum edit_error
edit_context_ref_put
  (struct edit_context* ctxt);

extern enum edit_error
edit_run
  (struct edit_context* ctxt);

#endif /* EDIT_CONTEXT_H */

