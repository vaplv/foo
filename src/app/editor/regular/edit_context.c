#include "app/core/app.h"
#include "app/core/app_cvar.h"
#include "app/core/app_imdraw.h"
#include "app/core/app_view.h"
#include "app/editor/regular/edit_context_c.h"
#include "app/editor/regular/edit_cvars.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_inputs.h"
#include "app/editor/regular/edit_load_save_commands.h"
#include "app/editor/regular/edit_move_commands.h"
#include "app/editor/regular/edit_object_management_commands.h"
#include "app/editor/regular/edit_picking.h"
#include "app/editor/regular/edit_tools.h"
#include "app/editor/edit_context.h"
#include "app/editor/edit_model_instance_selection.h"
#include "sys/math.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static inline void
draw_selection(struct edit_context* ctxt)
{
  assert(ctxt);
  if(true == ctxt->cvars.show_selection->value.boolean) {
    EDIT(draw_model_instance_selection(ctxt->instance_selection));
  }
}

static inline void
draw_grid(struct edit_context* ctxt)
{
  assert(ctxt);
  if(true == ctxt->cvars.show_grid->value.boolean) {
    APP(imdraw_grid
      (ctxt->app,
       APP_IMDRAW_FLAG_NONE,
       UINT32_MAX, /* invalid pick_id */
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
}

static void
draw_tools(struct edit_context* ctxt)
{
  const float size = 0.2f;
  int transform_flag = EDIT_TRANSFORM_NONE;
  float pivot_pos[3] = {0.f, 0.f, 0.f};
  assert(ctxt);

  EDIT(inputs_get_entity_transform_flag(ctxt->inputs, &transform_flag));
  EDIT(get_model_instance_selection_pivot(ctxt->instance_selection, pivot_pos));

  if(transform_flag & EDIT_TRANSFORM_SCALE) {
    EDIT(draw_scale_tool
      (ctxt->app, pivot_pos, size,
       (float[]){1.f, 0.f, 0.f},
       (float[]){0.f, 1.f, 0.f},
       (float[]){0.f, 0.f, 1.f}));
  }
  if(transform_flag & EDIT_TRANSFORM_TRANSLATE) {
    EDIT(draw_translate_tool
      (ctxt->app, pivot_pos, size,
       (float[]){1.f, 0.f, 0.f},
       (float[]){0.f, 1.f, 0.f},
       (float[]){0.f, 0.f, 1.f}));
  }
  if(transform_flag & EDIT_TRANSFORM_ROTATE) {
    EDIT(draw_rotate_tool
      (ctxt->app, pivot_pos, size,
       (float[]){1.f, 0.f, 0.f},
       (float[]){0.f, 1.f, 0.f},
       (float[]){0.f, 0.f, 1.f}));
  }
  if(transform_flag == EDIT_TRANSFORM_NONE) {
    EDIT(draw_pivot
      (ctxt->app, pivot_pos, 0.05f, ctxt->cvars.pivot_color->value.real3));
  }
}

static void
process_inputs(struct edit_context* ctxt)
{
  bool is_enabled = false;
  assert(ctxt);

  EDIT(inputs_is_enabled(ctxt->inputs, &is_enabled));
  if(is_enabled) {
    struct app_view* view = NULL;
    struct aosf44 view_transform;
    struct edit_inputs_config_desc inputs_cfg;

    /* Update inputs configuration */
    EDIT(inputs_get_config(ctxt->inputs, &inputs_cfg));
    inputs_cfg.mouse_sensitivity = ctxt->cvars.mouse_sensitivity->value.real;
    EDIT(inputs_set_config(ctxt->inputs, &inputs_cfg));

    /* Process inputs commands */
    EDIT(inputs_flush_commands(ctxt->inputs));

    /* Update main view */
    EDIT(inputs_get_view_transform(ctxt->inputs, &view_transform));
    APP(get_main_view(ctxt->app, &view));
    APP(raw_view_transform(view, &view_transform));
  }
}

static void
release_context(struct ref* ref)
{
  struct app* app = NULL;
  struct edit_context* ctxt = NULL;
  assert(ref);

  ctxt = CONTAINER_OF(ref, struct edit_context, ref);

  if(ctxt->instance_selection)
    EDIT(model_instance_selection_ref_put(ctxt->instance_selection));
  if(ctxt->inputs)
    EDIT(inputs_ref_put(ctxt->inputs));
  if(ctxt->picking)
    EDIT(picking_ref_put(ctxt->picking));

  EDIT(release_cvars(ctxt->app, &ctxt->cvars));

  /* Release edit builtin commands */
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
  ctxt = MEM_ALIGNED_ALLOC(edit_allocator, sizeof(struct edit_context), 16);
  if(!ctxt) {
    edit_err = EDIT_MEMORY_ERROR;
    goto error;
  }
  memset(ctxt, 0, sizeof(struct edit_context));
  ctxt->allocator = edit_allocator;
  ref_init(&ctxt->ref);
  APP(ref_get(app));
  ctxt->app = app;

  #define CALL(func) if(EDIT_NO_ERROR != (edit_err = func)) goto error
  CALL(edit_create_model_instance_selection
    (ctxt, &ctxt->instance_selection));
  CALL(edit_create_inputs(ctxt->app, ctxt->allocator, &ctxt->inputs));
  CALL(edit_create_picking
    (app,
     ctxt->inputs,
     ctxt->instance_selection,
     ctxt->allocator,
     &ctxt->picking));

  CALL(edit_setup_cvars(ctxt->app, &ctxt->cvars));
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
  enum edit_error edit_err = EDIT_NO_ERROR;

  if(UNLIKELY(!ctxt)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  draw_grid(ctxt);
  draw_selection(ctxt);
  draw_tools(ctxt);

  process_inputs(ctxt);

  edit_err = edit_process_picking(ctxt->picking);
  if(edit_err != EDIT_NO_ERROR)
    goto error;

exit:
  return edit_err;
error:
  goto exit;
}

EXPORT_SYM enum edit_error
edit_enable_inputs(struct edit_context* ctxt, bool is_enable)
{
  if(UNLIKELY(!ctxt))
     return EDIT_INVALID_ARGUMENT;

  if(is_enable) {
    EDIT(inputs_enable(ctxt->inputs));
  } else {
    EDIT(inputs_disable(ctxt->inputs));
  }

  return EDIT_NO_ERROR;
}

