#ifndef RSRC_CONTEXT_H
#define RSRC_CONTEXT_H

#include "resources/rsrc_error.h"

struct mem_allocator;
struct rsrc_context;

extern enum rsrc_error
rsrc_create_context
  (struct mem_allocator* allocator, /* May be NULL. */
   struct rsrc_context** out_ctxt);

extern enum rsrc_error
rsrc_context_ref_get
  (struct rsrc_context* ctxt);

extern enum rsrc_error
rsrc_context_ref_put
  (struct rsrc_context* ctxt);

extern enum rsrc_error
rsrc_get_error_string
  (struct rsrc_context* ctxt,
   const char** str_err);

extern enum rsrc_error
rsrc_flush_error
  (struct rsrc_context* ctxt);

#endif /* RSRC_CONTEXT_H */

