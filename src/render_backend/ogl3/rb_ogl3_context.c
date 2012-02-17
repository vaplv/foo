#include "render_backend/ogl3/rb_ogl3_context.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_context(struct ref* ref)
{
  struct rb_context* ctxt = NULL;
  assert(ref);

  ctxt = CONTAINER_OF(ref, struct rb_context, ref);
  MEM_FREE(ctxt->allocator, ctxt);
}

/*******************************************************************************
 *
 * Render backend context functions.
 *
 ******************************************************************************/
EXPORT_SYM int
rb_create_context
  (struct mem_allocator* specific_allocator,
   struct rb_context** out_ctxt)
{
  struct mem_allocator* allocator = NULL;
  struct rb_context* ctxt = NULL;
  int err = 0;

  if(!out_ctxt)
    goto error;

  allocator = specific_allocator ? specific_allocator : &mem_default_allocator;
  ctxt = MEM_CALLOC(allocator, 1, sizeof(struct rb_context));
  if(!ctxt)
    goto error;
  ctxt->allocator = allocator;
  ref_init(&ctxt->ref);

exit:
  if(ctxt)
    *out_ctxt = ctxt;
  return err;

error:
  if(ctxt) {
    RB(context_ref_put(ctxt));
    ctxt = NULL;
  }
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_context_ref_get(struct rb_context* ctxt)
{
  if(!ctxt)
    return -1;
  ref_get(&ctxt->ref);
  return 0;
}

EXPORT_SYM int
rb_context_ref_put(struct rb_context* ctxt)
{
  if(!ctxt)
    return -1;
  ref_put(&ctxt->ref, release_context);
  return 0;
}

