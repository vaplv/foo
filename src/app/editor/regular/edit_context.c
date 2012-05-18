#include "app/core/app.h"
#include "app/editor/regular/edit_context_c.h"
#include "app/editor/regular/edit_cvars.h"
#include "app/editor/regular/edit_load_save_commands.h"
#include "app/editor/regular/edit_model_instance_selection.h"
#include "app/editor/regular/edit_move_commands.h"
#include "app/editor/regular/edit_object_management_commands.h"
#include "app/editor/edit_context.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_context(struct ref* ref)
{
  struct app* app = NULL;
  struct edit_context* ctxt = NULL;
  assert(ref);

  ctxt = CONTAINER_OF(ref, struct edit_context, ref);

  EDIT(shutdown_model_instance_selection(ctxt));
  EDIT(release_cvars(ctxt));
  EDIT(release_move_commands(ctxt));
  EDIT(release_object_management_commands(ctxt));
  EDIT(release_load_save_commands(ctxt));

  app = ctxt->app;
  MEM_FREE(ctxt->allocator, ctxt);
  APP(ref_put(app));
}

/*******************************************************************************
 *
 * Context functions.
 *
 ******************************************************************************/
EXPORT_SYM enum edit_error
edit_create_context
  (struct app* app,
   struct mem_allocator* allocator,
   struct edit_context** out_ctxt)
{
  struct edit_context* ctxt = NULL;
  struct mem_allocator* edit_allocator =
    allocator ? allocator : &mem_default_allocator;
  enum edit_error edit_err = EDIT_NO_ERROR;

  if(UNLIKELY(!app || !out_ctxt)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }
  ctxt = MEM_CALLOC(edit_allocator, 1, sizeof(struct edit_context));
  if(!ctxt) {
    edit_err = EDIT_MEMORY_ERROR;
    goto error;
  }
  ctxt->allocator = edit_allocator;
  ref_init(&ctxt->ref);
  APP(ref_get(app));
  ctxt->app = app;

  #define CALL(func) if(EDIT_NO_ERROR != (edit_err = func)) goto error;
  CALL(edit_init_model_instance_selection(ctxt));
  CALL(edit_setup_cvars(ctxt));
  CALL(edit_setup_move_commands(ctxt));
  CALL(edit_setup_object_management_commands(ctxt));
  CALL(edit_setup_load_save_commands(ctxt));
  #undef CALL

exit:
  if(out_ctxt)
    *out_ctxt = ctxt;
  return edit_err;
error:
  if(ctxt) {
    EDIT(context_ref_put(ctxt));
    ctxt = NULL;
  }
  goto exit;
}

EXPORT_SYM enum edit_error
edit_context_ref_get(struct edit_context* ctxt)
{
  if(UNLIKELY(!ctxt))
    return EDIT_INVALID_ARGUMENT;
  ref_get(&ctxt->ref);
  return EDIT_NO_ERROR;
}

EXPORT_SYM enum edit_error
edit_context_ref_put(struct edit_context* ctxt)
{
  if(UNLIKELY(!ctxt))
    return EDIT_INVALID_ARGUMENT;
  ref_put(&ctxt->ref, release_context);
  return EDIT_NO_ERROR;
}

EXPORT_SYM enum edit_error
edit_run(struct edit_context* ctxt)
{
  if(UNLIKELY(!ctxt))
    return EDIT_INVALID_ARGUMENT;
  EDIT(draw_model_instance_selection(ctxt));
  return EDIT_NO_ERROR;
}

