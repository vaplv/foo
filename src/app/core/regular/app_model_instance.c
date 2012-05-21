#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_model_instance_c.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "app/core/app_world.h"
#include "renderer/rdr.h"
#include "renderer/rdr_model_instance.h"
#include "stdlib/sl.h"
#include "stdlib/sl_flat_map.h"
#include "stdlib/sl_string.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_model_instance(struct ref* ref)
{
  struct app_model_instance* instance = CONTAINER_OF
    (ref, struct app_model_instance, ref);
  struct app* app = instance->app;
  struct rdr_model_instance** render_instance_lstbuf = NULL;
  size_t len = 0;
  size_t i = 0;
  assert(app != NULL);

  if(instance->invoke_clbk)
    APP(invoke_callbacks
      (instance->app, APP_SIGNAL_DESTROY_MODEL_INSTANCE, instance));

  if(instance->model_instance_list) {
    if(instance->world) {
      APP(world_remove_model_instances(instance->world, 1, &instance));
    }
    SL(vector_buffer
       (instance->model_instance_list,
        &len,
        NULL,
        NULL,
        (void**)&render_instance_lstbuf));
    for(i = 0; i < len; ++i)
      RDR(model_instance_ref_put(render_instance_lstbuf[i]));
    SL(free_vector(instance->model_instance_list));
  }
  assert(instance->world == NULL);

  APP(release_object(instance->app, &instance->obj));

  list_del(&instance->model_node);
  list_del(&instance->world_node);
  APP(model_ref_put(instance->model));
  MEM_FREE(app->allocator, instance);
  APP(ref_put(app));
}

static void
release_model_instance_object(struct app_object* obj)
{
  assert(obj);
  APP(model_instance_ref_put(app_object_to_model_instance(obj)));
}

/*******************************************************************************
 *
 * Implementation of the public model instance functions.
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_remove_model_instance(struct app_model_instance* instance)
{
  bool is_registered = false;

  if(!instance)
    return APP_INVALID_ARGUMENT;

  APP(is_object_registered(instance->app, &instance->obj, &is_registered));
  if(is_registered == false)
    return APP_INVALID_ARGUMENT;

  if(ref_put(&instance->ref, release_model_instance) == 0) {
    if(instance->world)
      APP(world_remove_model_instances(instance->world, 1, &instance));
    APP(unregister_object(instance->app, &instance->obj));
  }

  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_model_instance_ref_get(struct app_model_instance* instance)
{
  if(!instance)
    return APP_INVALID_ARGUMENT;
  ref_get(&instance->ref);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_model_instance_ref_put(struct app_model_instance* instance)
{
  if(!instance)
    return APP_INVALID_ARGUMENT;
  ref_put(&instance->ref, release_model_instance);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_model_instance_get_model
  (struct app_model_instance* instance,
   struct app_model** out_model)
{
  if(!instance || !out_model)
    return APP_INVALID_ARGUMENT;
  *out_model = instance->model;
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_get_raw_model_instance_transform
  (const struct app_model_instance* instance,
   const struct aosf44** transform)
{
  if(!instance || !transform)
    return APP_INVALID_ARGUMENT;
  *transform = &instance->transform;
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_model_instance_name
  (const struct app_model_instance* instance,
   const char** name)
{
  if(!instance)
    return APP_INVALID_ARGUMENT;
  return app_object_name(instance->app, &instance->obj, name);
}

EXPORT_SYM enum app_error
app_set_model_instance_name
  (struct app_model_instance* instance,
   const char* name)
{
  if(!instance)
    return APP_INVALID_ARGUMENT;
  return app_set_object_name(instance->app, &instance->obj, name);
}

EXPORT_SYM enum app_error
app_translate_model_instances
  (struct app_model_instance* instance_list[],
   size_t nb_instances,
   bool local_translation,
   const float trans[3])
{
  size_t i = 0;

  if((nb_instances && !instance_list) || !trans)
    return APP_INVALID_ARGUMENT;
  if(!(trans[0]) & !(trans[1]) & !(trans[2]))
    return APP_NO_ERROR;

  if(local_translation) {
    const vf4_t vec = vf4_set(trans[0], trans[1], trans[2], 1.f);
    for(i = 0; i < nb_instances; ++i) {
      struct app_model_instance* instance = instance_list[i];
      struct rdr_model_instance** render_instance_list = NULL;
      size_t nb_render_instances = 0;
      /* Translate the instance. */
      instance->transform.c3 = aosf44_mulf4(&instance->transform, vec);
      /* Translate the render data of the instance. */
      SL(vector_buffer
        (instance_list[i]->model_instance_list,
         &nb_render_instances,
         NULL, NULL,
         (void**)&render_instance_list));
      RDR(translate_model_instances
        (render_instance_list,
         nb_render_instances,
         local_translation,
         trans));
    }
  } else {
    const vf4_t vec = vf4_set(trans[0], trans[1], trans[2], 0.f);
    for(i = 0; i < nb_instances; ++i) {
      struct app_model_instance* instance = instance_list[i];
      struct rdr_model_instance** render_instance_list = NULL;
      size_t nb_render_instances = 0;
      /* Translate the instance. */
      instance->transform.c3 = vf4_add(instance->transform.c3, vec);
      /* Translate the render data of the instance. */
      SL(vector_buffer
        (instance_list[i]->model_instance_list,
         &nb_render_instances,
         NULL, NULL,
         (void**)&render_instance_list));
      RDR(translate_model_instances
        (render_instance_list,
         nb_render_instances,
         local_translation,
         trans));
    }
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_rotate_model_instances
  (struct app_model_instance* instance_list[],
   size_t nb_instances,
   bool local_rotation,
   const float rotation[3])
{
  struct aosf33 f33;
  size_t i = 0;
  enum { PITCH, YAW, ROLL };

  if((nb_instances && !instance_list) || !rotation)
    return APP_INVALID_ARGUMENT;
  if((!rotation[PITCH]) & (!rotation[YAW]) & (!rotation[ROLL]))
     return RDR_NO_ERROR;

  aosf33_rotation(&f33, rotation[PITCH], rotation[YAW], rotation[ROLL]);
  if(local_rotation) {
    for(i = 0; i < nb_instances; ++i) {
      struct app_model_instance* instance = instance_list[i];
      struct rdr_model_instance** render_instance_list = NULL;
      size_t nb_render_instances = 0;
      const struct aosf33 tmp = {
        .c0 = instance->transform.c0,
        .c1 = instance->transform.c1,
        .c2 = instance->transform.c2
      };
      /* Rotate the instance. */
      aosf33_mulf33(&f33, &tmp, &f33);
      instance->transform.c0 = f33.c0;
      instance->transform.c1 = f33.c1;
      instance->transform.c2 = f33.c2;
      /* Rotate the render data of the instances. */
      SL(vector_buffer
       (instance_list[i]->model_instance_list,
        &nb_render_instances,
        NULL, NULL,
        (void**)&render_instance_list));
      RDR(rotate_model_instances
        (render_instance_list,
         nb_render_instances,
         local_rotation,
         rotation));
    }
  } else {
    const struct aosf44 f44 = {
      .c0 = f33.c0,
      .c1 = f33.c1,
      .c2 = f33.c2,
      .c3 = vf4_set(0.f, 0.f, 0.f, 1.f)
    };
    for(i = 0; i < nb_instances; ++i) {
      struct app_model_instance* instance = instance_list[i];
      struct rdr_model_instance** render_instance_list = NULL;
      size_t nb_render_instances = 0;
      /* Rotate the instance. */
      aosf44_mulf44(&instance->transform, &f44, &instance->transform);
      /* transform the render data of the instance. Note that we use the
       * transform function rather than the rotate one ine order to avoid the
       * costly recomputation of the rotation matrix. */
      SL(vector_buffer
       (instance_list[i]->model_instance_list,
        &nb_render_instances,
        NULL, NULL,
        (void**)&render_instance_list));
      RDR(transform_model_instances
        (render_instance_list,
         nb_render_instances,
         local_rotation,
         &f44));
    }
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_scale_model_instances
  (struct app_model_instance* instance_list[],
   size_t nb_instances,
   bool local_scale,
   const float scale[3])
{
  struct aosf33 f33;
  size_t i = 0;

  if((nb_instances && !instance_list) || !scale)
    return APP_INVALID_ARGUMENT;
  if((scale[0] == 1.f) & (scale[1] == 1.f) & (scale[2] == 1.f))
    return RDR_NO_ERROR;

  f33.c0 = vf4_set(scale[0], 0.f, 0.f, 0.f);
  f33.c1 = vf4_set(0.f, scale[1], 0.f, 0.f);
  f33.c2 = vf4_set(0.f, 0.f, scale[2], 0.f);

  if(local_scale) {
    for(i = 0; i < nb_instances; ++i) {
      struct app_model_instance* instance = instance_list[i];
      struct aosf33 tmp = {
        .c0 = instance->transform.c0,
        .c1 = instance->transform.c1,
        .c2 = instance->transform.c2
      };
      struct rdr_model_instance** render_instance_list = NULL;
      size_t nb_render_instances = 0;
      /* Scale the instance. */
      aosf33_mulf33(&tmp, &tmp, &f33);
      instance->transform.c0 = tmp.c0;
      instance->transform.c1 = tmp.c1;
      instance->transform.c2 = tmp.c2;
      /* Scale the render data of the instance. */
      SL(vector_buffer
        (instance_list[i]->model_instance_list,
         &nb_render_instances,
         NULL, NULL,
         (void**)&render_instance_list));
      RDR(scale_model_instances
        (render_instance_list,
         nb_render_instances,
         local_scale,
         scale));
    }
  } else {
    const struct aosf44 f44 = {
      .c0 = f33.c0,
      .c1 = f33.c1,
      .c2 = f33.c2,
      .c3 = vf4_set(0.f, 0.f, 0.f, 1.f)
    };
    for(i = 0; i < nb_instances; ++i) {
      struct app_model_instance* instance = instance_list[i];
      struct rdr_model_instance** render_instance_list = NULL;
      size_t nb_render_instances = 0;
      /* Scale the instance. */
      aosf44_mulf44(&instance->transform, &f44, &instance->transform);
      /* Scale the render data of the instance. */
      SL(vector_buffer
        (instance_list[i]->model_instance_list,
         &nb_render_instances,
         NULL, NULL,
         (void**)&render_instance_list));
      RDR(scale_model_instances
        (render_instance_list,
         nb_render_instances,
         local_scale,
         scale));
    }
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_move_model_instances
  (struct app_model_instance* instance_list[],
   size_t nb_instances,
   const float pos[3])
{
  size_t i = 0;

  if((nb_instances && !instance_list) || !pos)
    return APP_INVALID_ARGUMENT;

  for(i = 0; i < nb_instances; ++i) {
    struct app_model_instance* instance = instance_list[i];
    struct rdr_model_instance** render_instance_list = NULL;
    size_t nb_render_instances = 0;
    /* Move the instance. */
    instance->transform.c3 = vf4_set(pos[0], pos[1], pos[2], 1.f);
    /* Move the render data of the instance. */
    SL(vector_buffer
      (instance_list[i]->model_instance_list,
       &nb_render_instances,
       NULL, NULL,
       (void**)&render_instance_list));
    RDR(move_model_instances(render_instance_list, nb_render_instances, pos));
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_transform_model_instances
  (struct app_model_instance* instance_list[],
   size_t nb_instances,
   bool local_transform,
   const struct aosf44* transform)
{
  size_t i = 0;

  if((nb_instances && !instance_list) || !transform)
    return APP_INVALID_ARGUMENT;

  if(local_transform) {
    for(i = 0; i < nb_instances; ++i) {
      struct app_model_instance* instance = instance_list[i];
      struct rdr_model_instance** render_instance_list = NULL;
      size_t nb_render_instances = 0;
      /* Transform the instance. */
      aosf44_mulf44(&instance->transform, &instance->transform, transform);
      /* Transform the render data oftthe instance. */
      SL(vector_buffer
        (instance_list[i]->model_instance_list,
         &nb_render_instances,
         NULL, NULL,
         (void**)&render_instance_list));
      RDR(transform_model_instances
        (render_instance_list,
         nb_render_instances,
         local_transform,
         transform));
    }
  } else {
    for(i = 0; i < nb_instances; ++i) {
      struct app_model_instance* instance = instance_list[i];
      struct rdr_model_instance** render_instance_list = NULL;
      size_t nb_render_instances = 0;
      /* Transform the instance. */
      aosf44_mulf44(&instance->transform, transform, &instance->transform);
      /* Transform the render data oftthe instance. */
      SL(vector_buffer
        (instance_list[i]->model_instance_list,
         &nb_render_instances,
         NULL, NULL,
         (void**)&render_instance_list));
      RDR(transform_model_instances
        (render_instance_list,
         nb_render_instances,
         local_transform,
         transform));
    }
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_model_instance_world
  (struct app_model_instance* instance,
   struct app_world** world)
{
  if(!instance || !world)
    return APP_INVALID_ARGUMENT;
  *world = instance->world;
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_get_model_instance_aabb
  (const struct app_model_instance* instance,
   float min_bound[3],
   float max_bound[3])
{
  ALIGN(16) float tmp[4] = {0.f, 0.f, 0.f, 0.f};
  vf4_t vpt[8];
  vf4_t vmin;
  vf4_t vmax;
  vf4_t vmin_mask;
  vf4_t vmax_mask;
  size_t i = 0;

  if(UNLIKELY(!instance || !min_bound || !max_bound))
    return APP_INVALID_ARGUMENT;

  APP(get_model_aabb(instance->model, min_bound, max_bound));

  /* Find degenerated coordinates. */
  vmin_mask = vf4_mask
    (min_bound[0]==-FLT_MAX, min_bound[1]==-FLT_MAX, min_bound[2]==-FLT_MAX, 0);
  vmax_mask = vf4_mask
    (max_bound[0]==FLT_MAX, max_bound[1]==FLT_MAX, max_bound[2]==FLT_MAX, 0);

  /* Reset degenerated vertices <=> limit denormal computations. */
  min_bound[0] = min_bound[0] == -FLT_MAX ? 0.f : min_bound[0];
  min_bound[1] = min_bound[1] == -FLT_MAX ? 0.f : min_bound[1];
  min_bound[2] = min_bound[2] == -FLT_MAX ? 0.f : min_bound[2];
  max_bound[0] = max_bound[0] == FLT_MAX ? 0.f : max_bound[0];
  max_bound[1] = max_bound[1] == FLT_MAX ? 0.f : max_bound[1];
  max_bound[2] = max_bound[2] == FLT_MAX ? 0.f : max_bound[2];

  /* Build local space AABB vertices. */
  vpt[0] = vf4_set(min_bound[0], min_bound[1], min_bound[2], 1.f);
  vpt[1] = vf4_set(max_bound[0], min_bound[1], min_bound[2], 1.f);
  vpt[2] = vf4_set(max_bound[0], min_bound[1], max_bound[2], 1.f);
  vpt[3] = vf4_set(min_bound[0], min_bound[1], max_bound[2], 1.f);
  vpt[4] = vf4_set(min_bound[0], max_bound[1], min_bound[2], 1.f);
  vpt[5] = vf4_set(max_bound[0], max_bound[1], min_bound[2], 1.f);
  vpt[6] = vf4_set(max_bound[0], max_bound[1], max_bound[2], 1.f);
  vpt[7] = vf4_set(min_bound[0], max_bound[1], max_bound[2], 1.f);

  /* Compute the world AABB of the local AABB transformed in world space. */
  vpt[0] = aosf44_mulf4(&instance->transform, vpt[0]);
  vmin = vmax = vpt[0];
  for(i = 1; i < 8; ++i) {
    vpt[i] = aosf44_mulf4(&instance->transform, vpt[i]);
    vmin = vf4_min(vmin, vpt[i]);
    vmax = vf4_max(vmax, vpt[i]);
  }

  /* Set degenerated coordinates. */
  vmin = vf4_sel(vmin, vf4_set1(-FLT_MAX), vmin_mask);
  vmax = vf4_sel(vmax, vf4_set1(FLT_MAX), vmax_mask);

  /* Store world AABB. */
  vf4_store(tmp, vmin);
  min_bound[0] = tmp[0];
  min_bound[1] = tmp[1];
  min_bound[2] = tmp[2];
  vf4_store(tmp, vmax);
  max_bound[0] = tmp[0];
  max_bound[1] = tmp[1];
  max_bound[2] = tmp[2];

  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_get_model_instance_list_begin
  (struct app* app,
   struct app_model_instance_it* it,
   bool* is_end_reached)
{
  struct app_object** obj_list = NULL;
  size_t len = 0;

  if(UNLIKELY(!app || !it || !is_end_reached))
    return APP_INVALID_ARGUMENT;

  APP(get_object_list(app, APP_MODEL_INSTANCE, &len, &obj_list)); 
  *is_end_reached = (len == 0);
  if(*is_end_reached == false) {
    it->instance = app_object_to_model_instance(obj_list[0]);
    it->id = 0;
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_model_instance_it_next
  (struct app_model_instance_it* it,
   bool* is_end_reached)
{
  struct app_object** obj_list = NULL;
  size_t len = 0;

  if(UNLIKELY(!it || !is_end_reached || !it->instance))
     return APP_INVALID_ARGUMENT;

  APP(get_object_list(it->instance->app, APP_MODEL_INSTANCE, &len, &obj_list));
  ++it->id;
  *is_end_reached = (len <= it->id);
  if(*is_end_reached == false)
    it->instance = app_object_to_model_instance(obj_list[it->id]);

  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_get_model_instance_list_length(struct app* app, size_t* len)
{
  if(UNLIKELY(!app || !len))
    return APP_INVALID_ARGUMENT;
  APP(get_object_list(app, APP_MODEL_INSTANCE, len, NULL));
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_get_model_instance
  (struct app* app,
   const char* name,
   struct app_model_instance** instance)
{
  struct app_object* obj = NULL;
  enum app_error app_err = APP_NO_ERROR;

  if(!instance) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  app_err = app_get_object(app, APP_MODEL_INSTANCE, name, &obj);
  if(app_err != APP_NO_ERROR)
    goto error;

  if(obj) {
    *instance = app_object_to_model_instance(obj);
  } else {
    *instance = NULL;
  }
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_model_instance_name_completion
  (struct app* app,
   const char* instance_name,
   size_t instance_name_len,
   size_t* completion_list_len,
   const char** completion_list[])
{
  return app_object_name_completion
    (app,
     APP_MODEL_INSTANCE,
     instance_name,
     instance_name_len,
     completion_list_len,
     completion_list);
}

/*******************************************************************************
 *
 * Private model instance functions.
 *
 ******************************************************************************/
extern enum app_error
app_create_model_instance
  (struct app* app,
   const char* name,
   struct app_model_instance** out_instance)
{
  struct app_model_instance* instance = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app || !out_instance) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  instance = MEM_ALIGNED_ALLOC
    (app->allocator, sizeof(struct app_model_instance), 16);
  if(!instance) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  memset(instance, 0, sizeof(struct app_model_instance));

  ref_init(&instance->ref);
  list_init(&instance->model_node);
  list_init(&instance->world_node);
  aosf44_identity(&instance->transform);
  APP(ref_get(app));
  instance->app = app;

  app_err = app_init_object
    (app, 
     &instance->obj, 
     APP_MODEL_INSTANCE, 
     release_model_instance_object, 
     name ? name : "unamed");
  if(app_err != APP_NO_ERROR)
    goto error;

  sl_err = sl_create_vector
    (sizeof(struct rdr_model_instance*),
     ALIGNOF(struct rdr_model_instance*),
     app->allocator,
     &instance->model_instance_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
exit:
  if(out_instance)
    *out_instance = instance;
  return app_err;
error:
  if(instance) {
    APP(model_instance_ref_put(instance));
    instance = NULL;
  }
  goto exit;
}

struct app_model_instance*
app_object_to_model_instance(struct app_object* obj)
{
  assert(obj && obj->type == APP_MODEL_INSTANCE);
  return CONTAINER_OF(obj, struct app_model_instance, obj);
}

