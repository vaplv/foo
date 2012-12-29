#include "app/core/app.h"
#include "app/core/app_core.h"
#include "app/core/app_world.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_imgui.h"
#include "app/editor/regular/edit_picking.h"
#include "app/editor/edit_model_instance_selection.h"
#include "stdlib/sl_hash_table.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"

struct edit_picking {
  struct ref ref;
  struct app* app;
  struct edit_model_instance_selection* instance_selection;
  struct edit_imgui* imgui;
  struct mem_allocator* allocator;
  struct sl_hash_table* picked_instances_htbl;
};

/*******************************************************************************
 *
 * Helper functions
 *
 ******************************************************************************/
static size_t
hash_uint32(const void* ptr)
{
  return sl_hash(ptr, sizeof(uint32_t));
}

static bool
eq_uint32(const void* key0, const void* key1)
{
  const uint32_t a = *(const uint32_t*)key0;
  const uint32_t b = *(const uint32_t*)key1;
  return a == b;
}

static enum edit_error
pick_imdraw(struct edit_picking* picking, uint32_t pick_id)
{
  if(!picking)
    return EDIT_INVALID_ARGUMENT;
  EDIT(imgui_enable_item(picking->imgui, pick_id));
  return EDIT_NO_ERROR;
}

static enum edit_error
pick_world
  (struct edit_picking* picking,
   struct app_world* world,
   const uint32_t pick_id,
   const enum edit_selection_mode selection_mode)
{
  struct app_model_instance* inst = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum edit_error edit_err = EDIT_NO_ERROR;

  assert(picking && world);

  if(selection_mode == EDIT_SELECTION_MODE_NONE)
    goto exit;

  app_err = app_world_picked_model_instance(world, pick_id, &inst);
  if(app_err != APP_NO_ERROR) {
    edit_err = app_to_edit_error(app_err);
    goto error;
  }

  if(selection_mode == EDIT_SELECTION_MODE_XOR) {
    bool is_selected = false;

    edit_err = edit_is_model_instance_selected
      (picking->instance_selection, inst, &is_selected);
    if(edit_err != EDIT_NO_ERROR)
      goto error;

    if(is_selected == true) {
      void* data = NULL;
      /* If the instance was already selected and is not registered into the
       * picked_instance_htbl then this instance was previously selected and
       * thus we unselect it. */
      SL(hash_table_find(picking->picked_instances_htbl, &pick_id, &data));
      if(data == NULL) {
        edit_err = edit_unselect_model_instance
          (picking->instance_selection, inst);
        if(edit_err != EDIT_NO_ERROR)
          goto error;
      } 
    } else {
      /* If the instance was not already selected add it to the selection */
      const enum sl_error sl_err = sl_hash_table_insert
        (picking->picked_instances_htbl, &pick_id, &pick_id);
      if(sl_err != SL_NO_ERROR) {
        edit_err = sl_to_edit_error(sl_err);
        goto error;
      }
      edit_err = edit_select_model_instance
        (picking->instance_selection, inst);
      if(edit_err != EDIT_NO_ERROR)
        goto error;
    }
  } 

  if(selection_mode == EDIT_SELECTION_MODE_NEW) {
    EDIT(clear_model_instance_selection(picking->instance_selection));
    edit_err = edit_select_model_instance(picking->instance_selection, inst);
    if(edit_err != EDIT_NO_ERROR)
      goto error;
  }

exit:
  return edit_err;
error:
  goto exit;
}

static void
release_picking(struct ref* ref)
{
  struct edit_picking* picking = NULL;
  assert(ref);

  picking = CONTAINER_OF(ref, struct edit_picking, ref);
  EDIT(imgui_ref_put(picking->imgui));
  EDIT(model_instance_selection_ref_put(picking->instance_selection));
  APP(ref_put(picking->app));
  SL(free_hash_table(picking->picked_instances_htbl));
  MEM_FREE(picking->allocator, picking);
}

/*******************************************************************************
 *
 * Picking functions
 *
 ******************************************************************************/
enum edit_error
edit_create_picking
  (struct app* app,
   struct edit_imgui* imgui,
   struct edit_model_instance_selection* instance_selection,
   struct mem_allocator* allocator,
   struct edit_picking** out_picking)
{
  struct edit_picking* picking = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(UNLIKELY(!app || !instance_selection || !allocator || !out_picking)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  picking = MEM_CALLOC(allocator, 1, sizeof(struct edit_picking));
  if(!picking) {
    edit_err = EDIT_MEMORY_ERROR;
    goto error;
  }
  ref_init(&picking->ref);
  APP(ref_get(app));
  picking->app = app;
  EDIT(imgui_ref_get(imgui));
  picking->imgui = imgui;
  EDIT(model_instance_selection_ref_get(instance_selection));
  picking->instance_selection = instance_selection;
  picking->allocator = allocator;

  sl_err = sl_create_hash_table
    (sizeof(uint32_t),
     ALIGNOF(uint32_t),
     sizeof(uint32_t),
     ALIGNOF(uint32_t),
     hash_uint32,
     eq_uint32,
     allocator,
     &picking->picked_instances_htbl);
  if(sl_err != SL_NO_ERROR) {
    edit_err = sl_to_edit_error(sl_err);
    goto error;
  }

exit:
  if(out_picking)
    *out_picking = picking;
  return edit_err;
error:
  if(picking) {
    EDIT(picking_ref_put(picking));
    picking = NULL;
  }
  goto exit;
}

enum edit_error
edit_picking_ref_get(struct edit_picking* picking)
{
  if(UNLIKELY(!picking))
    return EDIT_INVALID_ARGUMENT;
  ref_get(&picking->ref);
  return EDIT_NO_ERROR;
}

enum edit_error
edit_picking_ref_put(struct edit_picking* picking)
{
  if(UNLIKELY(!picking))
    return EDIT_INVALID_ARGUMENT;
  ref_put(&picking->ref, release_picking);
  return EDIT_NO_ERROR;
}

enum edit_error
edit_process_picking
  (struct edit_picking* picking,
   const enum edit_selection_mode selection_mode)
{
  struct app_world* world = NULL;
  const app_pick_t* pick_list = NULL;
  size_t nb_picks = 0;
  size_t nb_picks_none = 0;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum edit_error edit_err = EDIT_NO_ERROR;

  if(UNLIKELY(!picking)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  APP(poll_picking(picking->app, &nb_picks, &pick_list));
  if(nb_picks == 0)
    goto exit;

  app_err = app_get_main_world(picking->app, &world);
  if(app_err != APP_NO_ERROR) {
    edit_err = app_to_edit_error(app_err);
    goto error;
  }

  SL(hash_table_clear(picking->picked_instances_htbl));

  for(i = 0; i < nb_picks; ++i) {
    const uint32_t pick_id = APP_PICK_ID_GET(pick_list[i]);
    const enum app_pick_group pick_group = APP_PICK_GROUP_GET(pick_list[i]);

    switch(pick_group) {
      case APP_PICK_GROUP_IMDRAW:
        edit_err = pick_imdraw(picking, pick_id);
        break;
      case APP_PICK_GROUP_WORLD:
        edit_err = pick_world(picking, world, pick_id, selection_mode);
        break;
      case APP_PICK_GROUP_NONE:
        ++nb_picks_none;
        break;
      default: assert(0); break; /* Unreachable code */
    }

    if(edit_err != EDIT_NO_ERROR)
      goto error;
  }

  if(nb_picks == nb_picks_none) {
    edit_err = edit_clear_model_instance_selection
      (picking->instance_selection);
  }

exit:
  return edit_err;
error:
  goto exit;
}

