#include "resources/regular/rsrc_context_c.h"
#include "resources/regular/rsrc_error_c.h"
#include "resources/rsrc_context.h"
#include "stdlib/sl_context.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>

EXPORT_SYM enum rsrc_error
rsrc_create_context(struct rsrc_context** out_ctxt)
{
  enum sl_error sl_err = SL_NO_ERROR;
  enum rsrc_error err = RSRC_NO_ERROR;
  struct rsrc_context* ctxt = NULL;

  if(!out_ctxt) {
    err = RSRC_INVALID_ARGUMENT;
    goto error;
  }

  ctxt = calloc(1, sizeof(struct rsrc_context));
  if(!ctxt) {
    err = RSRC_MEMORY_ERROR;
    goto error;
  }

  sl_err = sl_create_context(&ctxt->sl_ctxt);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }

exit:
  if(out_ctxt)
    *out_ctxt = ctxt;
  return err;

error:
  if(ctxt) {
    if(ctxt->sl_ctxt)
      SL(free_context(ctxt->sl_ctxt));
    free(ctxt);
    ctxt = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_free_context(struct rsrc_context* ctxt)
{
  enum sl_error sl_err = SL_NO_ERROR;
  enum rsrc_error err = RSRC_NO_ERROR;

  if(!ctxt) {
    err = RSRC_INVALID_ARGUMENT;
    goto error;
  }

  sl_err = sl_free_context(ctxt->sl_ctxt);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }

  free(ctxt);

exit:
  return err;

error:
  goto exit;
}

