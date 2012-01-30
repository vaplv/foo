#include "resources/regular/rsrc_context_c.h"
#include "resources/regular/rsrc_error_c.h"
#include "resources/regular/rsrc_font_c.h"
#include "resources/rsrc.h"
#include "resources/rsrc_context.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_context(struct ref* ref)
{
  struct rsrc_context* ctxt = NULL;
  assert(NULL != ref);

  ctxt = CONTAINER_OF(ref, struct rsrc_context, ref);

  RSRC(shutdown_font_library(ctxt));
  MEM_FREE(ctxt->allocator, ctxt);
}

/*******************************************************************************
 *
 * Context functions.
 *
 ******************************************************************************/
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
  ref_init(&ctxt->ref);

  err = rsrc_init_font_library(ctxt);
  if(err != RSRC_NO_ERROR)
    goto error;

exit:
  if(out_ctxt)
    *out_ctxt = ctxt;
  return err;

error:
  if(ctxt) {
    RSRC(context_ref_put(ctxt));
    ctxt = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_context_ref_get(struct rsrc_context* ctxt)
{
  if(!ctxt)
    return RSRC_INVALID_ARGUMENT;
  ref_get(&ctxt->ref);
  return RSRC_NO_ERROR;
}

EXPORT_SYM enum rsrc_error
rsrc_context_ref_put(struct rsrc_context* ctxt)
{
  if(!ctxt)
    return RSRC_INVALID_ARGUMENT;
  ref_put(&ctxt->ref, release_context);
  return RSRC_NO_ERROR;
}

