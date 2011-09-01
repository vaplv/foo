#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/rb.h"
#include "sys/sys.h"
#include <stdlib.h>
#include <string.h>

EXPORT_SYM int
rb_create_context(struct rb_context** out_ctxt)
{
  int err = 0;
  struct rb_context* ctxt = NULL;

  if(!out_ctxt)
    goto error;

  ctxt = malloc(sizeof(struct rb_context));
  if(!ctxt)
    goto error;

  ctxt->texture_binding_2d = 0;
  memset(ctxt->buffer_binding, 0, sizeof(ctxt->buffer_binding));
  ctxt->vertex_array_binding = 0;
  ctxt->current_program = 0;
  *out_ctxt = ctxt;

exit:
  return err;

error:
  free(ctxt);
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_free_context(struct rb_context* ctxt)
{
  int err = 0;
  if(!ctxt)
    return -1;

  free(ctxt);
  return err;
}

