#include "app/core/app.h"
#include "app/core/app_imdraw.h"
#include "app/core/app_model_instance.h"
#include "app/editor/regular/edit_context_c.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_model_instance_selection_c.h"
#include "app/editor/edit_context.h"
#include "app/editor/edit_model_instance_selection.h"
#include "maths/simd/aosf33.h"
#include "maths/simd/aosf44.h"
#include "stdlib/sl.h"
#include "stdlib/sl_hash_table.h"
#include "stdlib/sl_pair.h"
#include "sys/math.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <string.h>

struct edit_model_instance_selection {
  struct ref ref;
  struct edit_context* ctxt;
  struct sl_hash_table* instance_htbl;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static size_t
hash_ptr(const void* ptr)
{
  const void* p = *(const void**)ptr;
  return sl_hash(p, sizeof(const void*));
}

static bool
eq_ptr(const void* key0, const void* key1)
{
  const void* ptr0 = *(const void**)key0;
  const void* ptr1 = *(const void**)key1;
  return ptr0 == ptr1;
}

static void
on_destroy_model_instance(struct app_model_instance* instance, void*data)
{
  struct edit_model_instance_selection* selection = data;
  bool b = false;
  assert(instance && data);
  if(EDIT(is_model_instance_selected(selection, instance, &b)), b) {
    EDIT(unselect_model_instance(selection, instance));
  }
}

static void
release_regular_model_instance_selection(struct ref* ref)
{
  struct edit_model_instance_selection* selection = NULL;
  bool is_clbk_attached = false;
  assert(ref);

  selection = CONTAINER_OF(ref, struct edit_model_instance_selection, ref);
  if(selection->instance_htbl) {
    EDIT(clear_model_instance_selection(selection));
    SL(free_hash_table(selection->instance_htbl));
  }
  APP(is_callback_attached
    (selection->ctxt->app,
     APP_SIGNAL_DESTROY_MODEL_INSTANCE,
     APP_CALLBACK(on_destroy_model_instance),
     selection,
     &is_clbk_attached));
  if(is_clbk_attached) {
    APP(detach_callback
      (selection->ctxt->app,
       APP_SIGNAL_DESTROY_MODEL_INSTANCE,
       APP_CALLBACK(on_destroy_model_instance),
       selection));
  }
  MEM_FREE(selection->ctxt->allocator, selection);
}

static void
release_model_instance_selection(struct ref* ref)
{
  struct edit_context* ctxt = NULL;
  struct edit_model_instance_selection* selection = NULL;
  assert(ref);

  selection = CONTAINER_OF(ref, struct edit_model_instance_selection, ref);
  ctxt = selection->ctxt;
  release_regular_model_instance_selection(ref);
  EDIT(context_ref_put(ctxt));
}

/*******************************************************************************
 *
 * Model instance selection functions.
 *
 ******************************************************************************/
EXPORT_SYM enum edit_error
edit_create_model_instance_selection
  (struct edit_context* ctxt,
   struct edit_model_instance_selection** out_selection)
{
  enum edit_error edit_err = EDIT_NO_ERROR;

  edit_err = edit_regular_create_model_instance_selection(ctxt, out_selection);
  if(edit_err == EDIT_NO_ERROR)
    EDIT(context_ref_get(ctxt));
  return edit_err;
}

EXPORT_SYM enum edit_error
edit_model_instance_selection_ref_get
  (struct edit_model_instance_selection* selection)
{
  if(UNLIKELY(!selection))
    return EDIT_INVALID_ARGUMENT;
  ref_get(&selection->ref);
  return EDIT_NO_ERROR;
}

EXPORT_SYM enum edit_error
edit_model_instance_selection_ref_put
  (struct edit_model_instance_selection* selection)
{
  if(UNLIKELY(!selection))
    return EDIT_INVALID_ARGUMENT;
  ref_put(&selection->ref, release_model_instance_selection);
  return EDIT_NO_ERROR;
}

EXPORT_SYM enum edit_error
edit_select_model_instance
  (struct edit_model_instance_selection* selection,
   struct app_model_instance* instance)
{
  void* ptr = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(UNLIKELY(!selection || !instance)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }
  SL(hash_table_find(selection->instance_htbl, &instance, &ptr));
  if(ptr != NULL) {
    const char* instance_name = NULL;
    APP(model_instance_name(instance, &instance_name));
    APP(log(selection->ctxt->app, APP_LOG_INFO,
      "the instance `%s' is already selected\n", instance_name));
  } else {
    sl_err = sl_hash_table_insert
      (selection->instance_htbl, &instance, &instance);
    if(sl_err != SL_NO_ERROR) {
      edit_err = sl_to_edit_error(sl_err);
      goto error;
    }
  }
exit:
  return edit_err;
error:
  goto exit;
}

EXPORT_SYM enum edit_error
edit_unselect_model_instance
  (struct edit_model_instance_selection* selection,
   struct app_model_instance* instance)
{
  enum edit_error edit_err = EDIT_NO_ERROR;
  struct app_model_instance** selected_instance = NULL;
  size_t i = 0;

  if(UNLIKELY(!selection || !instance)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }
  SL(hash_table_find
    (selection->instance_htbl, &instance, (void**)&selected_instance));
  if(!selected_instance) {
    const char* name = NULL;
    APP(model_instance_name(instance, &name));
    APP(log(selection->ctxt->app, APP_LOG_ERROR,
      "the instance `%s' is not selected\n", name));
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }
  assert(instance == *selected_instance);
  SL(hash_table_erase(selection->instance_htbl, &instance, &i));
  assert(i == 1);
exit:
  return edit_err;
error:
  goto exit;
}

EXPORT_SYM enum edit_error
edit_is_model_instance_selected
  (struct edit_model_instance_selection* selection,
   struct app_model_instance* instance,
   bool* is_selected)
{
  void* ptr = NULL;

  if(UNLIKELY(!selection || !instance || !is_selected))
    return EDIT_INVALID_ARGUMENT;
  SL(hash_table_find(selection->instance_htbl, &instance, &ptr));
  *is_selected = (ptr != NULL);
  return EDIT_NO_ERROR;
}

EXPORT_SYM enum edit_error
edit_remove_selected_model_instances
  (struct edit_model_instance_selection* selection)
{
  enum edit_error edit_err = EDIT_NO_ERROR;
  struct sl_hash_table_it it;
  bool is_end_reached = false;
  memset(&it, 0, sizeof(it));

  if(UNLIKELY(!selection)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  SL(hash_table_begin(selection->instance_htbl, &it, &is_end_reached));
  while(is_end_reached == false) {
    struct app_model_instance* instance =
      *(struct app_model_instance**)it.pair.data;
    EDIT(unselect_model_instance(selection, instance));
    APP(remove_model_instance(instance));

    /* Always retrieve the beginning of the hash table since it is updated by
     * the unselect_model_instance function. */
    SL(hash_table_begin(selection->instance_htbl, &it, &is_end_reached));
  }
exit:
  return edit_err;
error:
  goto exit;
}

EXPORT_SYM enum edit_error
edit_clear_model_instance_selection
  (struct edit_model_instance_selection* selection)
{
  if(UNLIKELY(!selection))
    return EDIT_INVALID_ARGUMENT;
  SL(hash_table_clear(selection->instance_htbl));
  return EDIT_NO_ERROR;
}

EXPORT_SYM enum edit_error
edit_get_model_instance_selection_pivot
  (struct edit_model_instance_selection* selection,
   float pivot[3])
{
  struct sl_hash_table_it it;
  size_t nb_instances = 0;
  float rcp_nb_instances = 0.f;
  bool is_end_reached = false;
  memset(&it, 0, sizeof(it));

  if(UNLIKELY(!selection || !pivot))
    return EDIT_INVALID_ARGUMENT;

  nb_instances = 0;
  pivot[0] = pivot[1] = pivot[2] = 0.f;
  SL(hash_table_begin(selection->instance_htbl, &it, &is_end_reached));
  while(is_end_reached == false) {
    float min_bound[3] = {0.f, 0.f, 0.f};
    float max_bound[3] = {0.f, 0.f, 0.f};
    const struct app_model_instance* instance =
      *(struct app_model_instance**)it.pair.data;

    APP(get_model_instance_aabb(instance, min_bound, max_bound));
    pivot[0] += (min_bound[0] + max_bound[0]) * 0.5f;
    pivot[1] += (min_bound[1] + max_bound[1]) * 0.5f;
    pivot[2] += (min_bound[2] + max_bound[2]) * 0.5f;
    ++nb_instances;

    SL(hash_table_it_next(&it, &is_end_reached));
  }
  rcp_nb_instances = 1.f / (float)nb_instances;
  pivot[0] *= rcp_nb_instances;
  pivot[1] *= rcp_nb_instances;
  pivot[2] *= rcp_nb_instances;
  return EDIT_NO_ERROR;
}

EXPORT_SYM enum edit_error
edit_translate_model_instance_selection
  (struct edit_model_instance_selection* selection,
   const float translation[3])
{
  struct sl_hash_table_it it;
  bool is_end_reached = false;

  if(UNLIKELY(!selection || !translation))
    return EDIT_INVALID_ARGUMENT;
  if((!translation[0]) & (!translation[1]) & (!translation[2]))
    return EDIT_NO_ERROR;

  SL(hash_table_begin(selection->instance_htbl, &it, &is_end_reached));
  while(is_end_reached == false) {
    struct app_model_instance* instance =
      *(struct app_model_instance**)it.pair.data;
    APP(translate_model_instances(&instance, 1, false, translation));
    SL(hash_table_it_next(&it, &is_end_reached));
  }
  return EDIT_NO_ERROR;
}

EXPORT_SYM enum edit_error
edit_rotate_model_instance_selection
  (struct edit_model_instance_selection* selection,
   bool local_rotation,
   const float rotation[3])
{
  struct sl_hash_table_it it;
  bool is_end_reached = false;

  if(UNLIKELY(!selection || !rotation))
    return EDIT_INVALID_ARGUMENT;
  if((!rotation[0]) & (!rotation[1]) & (!rotation[2]))
    return EDIT_NO_ERROR;

  if(local_rotation == false) {
    SL(hash_table_begin(selection->instance_htbl, &it, &is_end_reached));
    while(is_end_reached == false) {
      struct app_model_instance* instance =
        *(struct app_model_instance**)it.pair.data;
      APP(rotate_model_instances(&instance, 1, false, rotation));
      SL(hash_table_it_next(&it, &is_end_reached));
    }
  } else {
    struct aosf44 transform;
    struct aosf33 rot_3x3;
    vf4_t translation;
    float pivot[3] = {0.f, 0.f, 0.f};

    EDIT(get_model_instance_selection_pivot(selection, pivot));

    translation = vf4_minus(vf4_set(pivot[0], pivot[1], pivot[2], 0.f));
    aosf33_rotation(&rot_3x3, rotation[0], rotation[1], rotation[2]);
    transform.c0 = rot_3x3.c0;
    transform.c1 = rot_3x3.c1;
    transform.c2 = rot_3x3.c2;
    transform.c3 = vf4_xyzd(aosf33_mulf3(&rot_3x3, translation), vf4_set1(1.f));
    transform.c3 = vf4_sub(transform.c3, translation);

    SL(hash_table_begin(selection->instance_htbl, &it, &is_end_reached));
    while(is_end_reached == false) {
      struct app_model_instance* instance =
        *(struct app_model_instance**)it.pair.data;
      APP(transform_model_instances(&instance, 1, false, &transform));
      SL(hash_table_it_next(&it, &is_end_reached));
    }

  }
  return EDIT_NO_ERROR;
}

EXPORT_SYM enum edit_error
edit_scale_model_instance_selection
  (struct edit_model_instance_selection* selection,
   bool local_scale,
   const float scale[3])
{
  struct sl_hash_table_it it;
  bool is_end_reached = false;

  if(UNLIKELY(!selection || !scale))
    return EDIT_INVALID_ARGUMENT;
  if((scale[0] == 1.f) & (scale[1] == 1.f) & (scale[2] == 1.f))
    return EDIT_NO_ERROR;

  if(local_scale == false) {
    SL(hash_table_begin(selection->instance_htbl, &it, &is_end_reached));
    while(is_end_reached == false) {
      struct app_model_instance* instance =
        *(struct app_model_instance**)it.pair.data;
      APP(scale_model_instances(&instance, 1, false, scale));
      SL(hash_table_it_next(&it, &is_end_reached));
    }
  } else {
    struct aosf44 transform;
    struct aosf33 f33;
    vf4_t translation;
    float pivot[3] = {0.f, 0.f, 0.f};

    EDIT(get_model_instance_selection_pivot(selection, pivot));

    translation = vf4_minus(vf4_set(pivot[0], pivot[1], pivot[2], 0.f));
    transform.c0 = f33.c0 = vf4_set(scale[0], 0.f, 0.f, 0.f);
    transform.c1 = f33.c1 = vf4_set(0.f, scale[1], 0.f, 0.f);
    transform.c2 = f33.c2 = vf4_set(0.f, 0.f, scale[2], 0.f);
    transform.c3 = vf4_xyzd(aosf33_mulf3(&f33, translation), vf4_set1(1.f));
    transform.c3 = vf4_sub(transform.c3, translation);

    SL(hash_table_begin(selection->instance_htbl, &it, &is_end_reached));
    while(is_end_reached == false) {
      struct app_model_instance* instance =
        *(struct app_model_instance**)it.pair.data;
      APP(transform_model_instances(&instance, 1, false, &transform));
      SL(hash_table_it_next(&it, &is_end_reached));
    }
  }
  return EDIT_NO_ERROR;
}

EXPORT_SYM enum edit_error
edit_transform_model_instance_selection
  (struct edit_model_instance_selection* selection,
   bool local_transformation,
   const struct aosf44* transform)
{
  struct sl_hash_table_it it;
  bool is_end_reached = false;

  if(UNLIKELY(!selection || !transform))
    return EDIT_INVALID_ARGUMENT;

  if(local_transformation == false) {
    SL(hash_table_begin(selection->instance_htbl, &it, &is_end_reached));
    while(is_end_reached == false) {
      struct app_model_instance* instance =
        *(struct app_model_instance**)it.pair.data;
      APP(transform_model_instances(&instance, 1, false, transform));
      SL(hash_table_it_next(&it, &is_end_reached));
    }
  } else {
    struct aosf44 tmp_4x4;
    vf4_t translation;
    float pivot[3] = {0.f, 0.f, 0.f};

    EDIT(get_model_instance_selection_pivot(selection, pivot));

    translation = vf4_minus(vf4_set(pivot[0], pivot[1], pivot[2], -1.f));
    tmp_4x4.c0 = transform->c0;
    tmp_4x4.c1 = transform->c1;
    tmp_4x4.c2 = transform->c2;
    tmp_4x4.c3 = aosf44_mulf4(transform, translation);
    tmp_4x4.c3 = vf4_sub(tmp_4x4.c3, translation);

    SL(hash_table_begin(selection->instance_htbl, &it, &is_end_reached));
    while(is_end_reached == false) {
      struct app_model_instance* instance =
        *(struct app_model_instance**)it.pair.data;
      APP(transform_model_instances(&instance, 1, false, transform));
      SL(hash_table_it_next(&it, &is_end_reached));
    }
  }
  return EDIT_NO_ERROR;
}

EXPORT_SYM enum edit_error
edit_draw_model_instance_selection
  (struct edit_model_instance_selection* selection)
{
  struct sl_hash_table_it it;
  float pivot[3] = { 0.f, 0.f, 0.f };
  float rcp_nb_selected_instances = 0.f;
  size_t nb_selected_instances = 0;
  enum edit_error edit_err = EDIT_NO_ERROR;
  bool is_end_reached = false;
  memset(&it, 0, sizeof(it));

  if(UNLIKELY(!selection)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }
  /* Draw the abb of theselected instances. */
  pivot[0] = pivot[1] = pivot[2] = 0.f;
  nb_selected_instances = 0;
  SL(hash_table_begin(selection->instance_htbl, &it, &is_end_reached));
  while(is_end_reached == false) {
    float min_bound[3] = {0.f, 0.f, 0.f};
    float max_bound[3] = {0.f, 0.f, 0.f};
    float size[3] = {0.f, 0.f, 0.f};
    float pos[3] = {0.f, 0.f, 0.f};
    const struct app_model_instance* instance =
      *(struct app_model_instance**)it.pair.data;

    APP(get_model_instance_aabb(instance, min_bound, max_bound));
    pivot[0] += pos[0] = (min_bound[0] + max_bound[0]) * 0.5f;
    pivot[1] += pos[1] = (min_bound[1] + max_bound[1]) * 0.5f;
    pivot[2] += pos[2] = (min_bound[2] + max_bound[2]) * 0.5f;
    size[0] = max_bound[0] - min_bound[0];
    size[1] = max_bound[1] - min_bound[1];
    size[2] = max_bound[2] - min_bound[2];
    APP(imdraw_parallelepiped
      (selection->ctxt->app,
       APP_IMDRAW_FLAG_NONE,
       pos,
       size,
       (float[]){0.f, 0.f, 0.f}, /* Rotation */ 
       (float[]){0.5f, 0.5f, 0.5f, 0.15f}, /* Solid color */
       (float[]){0.75f, 0.75f, 0.75f, 1.f})); /* Wire color */
    SL(hash_table_it_next(&it, &is_end_reached));
    ++nb_selected_instances;
  }
  /* Draw the pivot. */
  rcp_nb_selected_instances = 1.f / (float) nb_selected_instances;
  pivot[0] *= rcp_nb_selected_instances;
  pivot[1] *= rcp_nb_selected_instances;
  pivot[2] *= rcp_nb_selected_instances;
  #define DRAW_CIRCLE(pitch, yaw, roll) \
    APP(imdraw_ellipse \
      (selection->ctxt->app, \
       APP_IMDRAW_FLAG_UPPERMOST_LAYER | APP_IMDRAW_FLAG_FIXED_SCREEN_SIZE, \
       pivot, \
       (float[]){1.f, 1.f},/* Size */ \
       (float[]){(pitch), (yaw), (roll)}, /* Rotation */ \
       (float[]){1.f, 1.f, 0.f})) /* Color */
  DRAW_CIRCLE(0.f, 0.f, 0.f);
  DRAW_CIRCLE(PI * 0.5f, 0.f, 0.f);
  DRAW_CIRCLE(0.f, PI * 0.5f, 0.f);
  #undef DRAW_CIRCLE

exit:
  return edit_err;
error:
  goto exit;
}

/*******************************************************************************
 *
 * Private model selection functions.
 *
 ******************************************************************************/
enum edit_error
edit_regular_create_model_instance_selection
  (struct edit_context* ctxt,
   struct edit_model_instance_selection** out_selection)
{
  struct edit_model_instance_selection* selection = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum edit_error edit_err = EDIT_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(UNLIKELY(!ctxt || !out_selection)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }
  selection = MEM_CALLOC
    (ctxt->allocator, 1, sizeof(struct edit_model_instance_selection));
  if(!selection) {
    edit_err = EDIT_MEMORY_ERROR;
    goto error;
  }
  ref_init(&selection->ref);
  selection->ctxt = ctxt;

  sl_err = sl_create_hash_table
    (sizeof(struct app_model_instance*),
     ALIGNOF(struct app_model_instance*),
     sizeof(struct app_model_instance*),
     ALIGNOF(struct app_model_instance*),
     hash_ptr,
     eq_ptr,
     selection->ctxt->allocator,
     &selection->instance_htbl);
  if(sl_err != SL_NO_ERROR) {
    edit_err = sl_to_edit_error(sl_err);
    goto error;
  }
  app_err = app_attach_callback
    (selection->ctxt->app,
     APP_SIGNAL_DESTROY_MODEL_INSTANCE,
     APP_CALLBACK(on_destroy_model_instance),
     selection);
  if(app_err != APP_NO_ERROR) {
    edit_err = app_to_edit_error(app_err);
    goto error;
  }

exit:
  if(out_selection)
    *out_selection = selection;
  return edit_err;
error:
  if(selection) {
    EDIT(regular_model_instance_selection_ref_put(selection));
    selection = NULL;
  }
  goto exit;
}

enum edit_error
edit_regular_model_instance_selection_ref_put
  (struct edit_model_instance_selection* selection)
{
  if(UNLIKELY(!selection))
    return EDIT_INVALID_ARGUMENT;
  ref_put(&selection->ref, release_regular_model_instance_selection);
  return EDIT_NO_ERROR;
}

