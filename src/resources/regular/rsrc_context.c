#include "resources/regular/rsrc_context_c.h"
#include "resources/regular/rsrc_error_c.h"
#include "resources/rsrc_context.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>

EXPORT_SYM enum rsrc_error
rsrc_create_context(struct rsrc_context** out_ctxt)
{
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

exit:
  if(out_ctxt)
    *out_ctxt = ctxt;
  return err;

error:
  if(ctxt) {
    free(ctxt);
    ctxt = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_free_context(struct rsrc_context* ctxt)
{
  enum rsrc_error err = RSRC_NO_ERROR;

  if(!ctxt) {
    err = RSRC_INVALID_ARGUMENT;
    goto error;
  }
  free(ctxt);

exit:
  return err;

error:
  goto exit;
}

