#include "app/core/app.h"
#include "app/core/app_cvar.h"
#include "app/core/app_imdraw.h"
#include "app/editor/regular/edit_context_c.h"
#include "app/editor/regular/edit_cvars.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_load_save_commands.h"
#include "app/editor/regular/edit_model_instance_selection_c.h"
#include "app/editor/regular/edit_move_commands.h"
#include "app/editor/regular/edit_object_management_commands.h"
#include "app/editor/edit_context.h"
#include "app/editor/edit_model_instance_selection.h"
#include "sys/math.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "window_manager/wm.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_input.h"
#include <assert.h>

/*******************************************************************************
 *
 * Inputs
 *
 ******************************************************************************/
static void
key_clbk(enum wm_key key, enum wm_state state, void* data)
{
  struct edit_context* ctxt = data;
  assert(data);

  if(state != WM_PRESS)
    return;

  switch(key) {
    case WM_KEY_R:
      ctxt->states.move_flag = EDIT_MOVE_ROTATE;
      break;
    case WM_KEY_T:
      ctxt->states.move_flag = EDIT_MOVE_TRANSLATE;
      break;
    case WM_KEY_E:
      ctxt->states.move_flag = EDIT_MOVE_SCALE;
      break;
    case WM_KEY_P:
      ctxt->states.move_flag = EDIT_MOVE_NONE;
      break;
    default:
      /* Do nothing */
      break;
  }
}

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
  struct wm_device* wm = NULL;
  bool b = false;
  assert(ref);

  ctxt = CONTAINER_OF(ref, struct edit_context, ref);

  if(ctxt->instance_selection)
    EDIT(regular_model_instance_selection_ref_put(ctxt->instance_selection));

  EDIT(enable_inputs(ctxt, false));
  EDIT(release_cvars(ctxt));
  EDIT(release_move_commands(ctxt));
  EDIT(release_object_management_commands(ctxt));
  EDIT(release_load_save_commands(ctxt));

  APP(get_window_manager_device(ctxt->app, &wm));
  if(WM(is_key_callback_attached(wm, key_clbk, ctxt, &b)), b) {
    WM(detach_key_callback(wm, key_clbk, ctxt));
  }

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

  #define CALL(func) if(EDIT_NO_ERROR != (edit_err = func)) goto error
  CALL(edit_regular_create_model_instance_selection
    (ctxt, &ctxt->instance_selection));
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

  if(true == ctxt->cvars.show_selection->value.boolean)
    EDIT(draw_model_instance_selection(ctxt->instance_selection));
  if(true == ctxt->cvars.show_grid->value.boolean) {
    APP(imdraw_grid
      (ctxt->app,
       APP_IMDRAW_FLAG_NONE,
       (float[]){0.f, 0.f, 0.f}, /* pos */
       (float[]){
          ctxt->cvars.grid_size->value.real,
          ctxt->cvars.grid_size->value.real
       },
       (float[]){PI * 0.5f, 0.f, 0.f}, /* rotation */
       (unsigned int[]){ /* ndiv */
          ctxt->cvars.grid_ndiv->value.integer,
          ctxt->cvars.grid_ndiv->value.integer
       },
       (unsigned int[]){ /* nsubdiv */
          ctxt->cvars.grid_nsubdiv->value.integer,
          ctxt->cvars.grid_nsubdiv->value.integer
       },
       (float[]){0.25f, 0.25f, 0.25f}, /* color */
       (float[]){0.15f, 0.15f, 0.15f}, /* subcolor */
       (float[]){0.f, 0.f, 1.f}, /* vaxis_color */
       (float[]){1.f, 0.f, 0.f}));  /* haxis_color */
  }
  return EDIT_NO_ERROR;
}

EXPORT_SYM enum edit_error
edit_enable_inputs(struct edit_context* ctxt, bool is_enable)
{
  struct wm_device* wm = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;
  bool is_clbk_attached = false;

  if(!ctxt) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  APP(get_window_manager_device(ctxt->app, &wm));
  WM(is_key_callback_attached(wm, key_clbk, ctxt, &is_clbk_attached));

  if(is_enable && !is_clbk_attached) {
    wm_err = wm_attach_key_callback(wm, key_clbk, ctxt);
    if(wm_err != WM_NO_ERROR) {
      edit_err = wm_to_edit_error(wm_err);
      goto error;
    }
  }
  if(!is_enable && is_clbk_attached) {
    wm_err = wm_detach_key_callback(wm, key_clbk, ctxt);
    if(wm_err != WM_NO_ERROR) {
      edit_err = wm_to_edit_error(wm_err);
      goto error;
    }
  }
  ctxt->process_inputs = is_enable;

exit:
  return edit_err;

error:
  if(ctxt) {
    if(wm) {
      bool b = false;
      WM(is_key_callback_attached(wm, key_clbk, ctxt, &b));
      if(!b && is_clbk_attached) {
        WM(attach_key_callback(wm, key_clbk, ctxt));
      }
      if(b && !is_clbk_attached) {
        WM(detach_key_callback(wm, key_clbk, ctxt));
      }
    }
  }
  goto exit;
}

