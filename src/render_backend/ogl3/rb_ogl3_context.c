#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <stdlib.h>
#include <string.h>

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

exit:
  if(ctxt)
    *out_ctxt = ctxt;
  return err;

error:
  if(ctxt) {
    MEM_FREE(allocator, ctxt);
    ctxt = NULL;
  }
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_free_context(struct rb_context* ctxt)
{
  int err = 0;
  if(!ctxt)
    return -1;
  MEM_FREE(ctxt->allocator, ctxt);
  return err;
}

