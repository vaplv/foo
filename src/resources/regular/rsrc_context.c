#include "resources/regular/rsrc_context_c.h"
#include "resources/regular/rsrc_error_c.h"
#include "resources/regular/rsrc_font_c.h"
#include "resources/rsrc.h"
#include "resources/rsrc_context.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
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
enum rsrc_error
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

enum rsrc_error
rsrc_context_ref_get(struct rsrc_context* ctxt)
{
  if(!ctxt)
    return RSRC_INVALID_ARGUMENT;
  ref_get(&ctxt->ref);
  return RSRC_NO_ERROR;
}

enum rsrc_error
rsrc_context_ref_put(struct rsrc_context* ctxt)
{
  if(!ctxt)
    return RSRC_INVALID_ARGUMENT;
  ref_put(&ctxt->ref, release_context);
  return RSRC_NO_ERROR;
}

enum rsrc_error
rsrc_get_error_string(struct rsrc_context* ctxt, const char** str_err)
{
  if(!ctxt || !str_err)
    return RSRC_INVALID_ARGUMENT;
  *str_err = ctxt->errbuf_id == 0 ? NULL : ctxt->errbuf;
  return RSRC_NO_ERROR;
}

enum rsrc_error
rsrc_flush_error(struct rsrc_context* ctxt)
{
  if(!ctxt)
    return RSRC_INVALID_ARGUMENT;
  ctxt->errbuf_id = 0;
  ctxt->errbuf[0] = '\0';
  return RSRC_NO_ERROR;
}

/*******************************************************************************
 *
 * Private context functions.
 *
 ******************************************************************************/
enum rsrc_error
rsrc_print_error(struct rsrc_context* ctxt, const char* fmt, ...)
{
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;

  if(!ctxt || !fmt) {
    rsrc_err = RSRC_INVALID_ARGUMENT;
    goto error;
  }
  if(ctxt->errbuf_id < RSRC_ERRBUF_LEN - 1) {
    va_list vargs_list;
    const size_t size = RSRC_ERRBUF_LEN - ctxt->errbuf_id - 1; /*-1<=>\0 char*/
    int i = 0;

    va_start(vargs_list, fmt);

    i = vsnprintf(ctxt->errbuf + ctxt->errbuf_id, size, fmt, vargs_list);
    assert(i > 0);
    if((size_t)i >= size) {
      ctxt->errbuf_id = RSRC_ERRBUF_LEN;
      ctxt->errbuf[RSRC_ERRBUF_LEN - 1] = '\0';
    } else {
      ctxt->errbuf_id += i;
    }

    va_end(vargs_list);
  }

exit:
  return rsrc_err;
error:
  goto exit;
}

