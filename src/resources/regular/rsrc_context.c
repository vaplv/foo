#include "resources/regular/rsrc_context_c.h"
#include "resources/regular/rsrc_error_c.h"
#include "resources/rsrc_context.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>

EXPORT_SYM enum rsrc_error
rsrc_create_context
  (struct mem_allocator* specific_allocator, 
   struct rsrc_context** out_ctxt)
{
  struct mem_allocator* allocator = NULL;
  struct rsrc_context* ctxt = NULL;
  enum rsrc_error err = RSRC_NO_ERROR;

  if(!out_ctxt) {
    err = RSRC_INVALID_ARGUMENT;
    goto error;
  }
  allocator = specific_allocator ? specific_allocator : &mem_default_allocator;
  ctxt = MEM_CALLOC(allocator, 1, sizeof(struct rsrc_context));
  if(!ctxt) {
    err = RSRC_MEMORY_ERROR;
    goto error;
  }
  ctxt->allocator = allocator;

exit:
  if(out_ctxt)
    *out_ctxt = ctxt;
  return err;

error:
  if(ctxt) {
    MEM_FREE(allocator, ctxt);
    ctxt = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_free_context(struct rsrc_context* ctxt)
{
  struct mem_allocator* allocator = NULL;
  enum rsrc_error err = RSRC_NO_ERROR;

  if(!ctxt) {
    err = RSRC_INVALID_ARGUMENT;
    goto error;
  }
  allocator = ctxt->allocator;
  MEM_FREE(allocator, ctxt);

exit:
  return err;

error:
  goto exit;
}

