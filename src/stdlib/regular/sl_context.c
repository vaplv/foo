#include "stdlib/regular/sl_context_c.h"
#include "stdlib/sl_context.h"
#include "sys/sys.h"
#include <stdlib.h>

EXPORT_SYM enum sl_error
sl_create_context
  (struct sl_context** out_ctxt)
{
  struct sl_context* ctxt = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!out_ctxt) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  ctxt = calloc(1, sizeof(struct sl_context));
  if(!ctxt) {
    err = SL_MEMORY_ERROR;
    goto error;
  }

exit:
  if(out_ctxt)
    *out_ctxt = ctxt;
  return err;

error:
  if(!ctxt) {
    free(ctxt);
    ctxt = NULL;
  }
  goto exit;
}

EXPORT_SYM enum sl_error
sl_free_context
  (struct sl_context* ctxt)
{
  if(!ctxt)
    return SL_INVALID_ARGUMENT;

  free(ctxt);

  return SL_NO_ERROR;
}

