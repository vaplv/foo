#include "app/core/regular/app_core_c.h"
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

/* return true if the box is infinite. */
static bool
get_model_instance_obb
  (const struct app_model_instance* instance,
   vf4_t* restrict position,
   vf4_t* restrict extend_x,
   vf4_t* restrict extend_y,
   vf4_t* restrict extend_z)
{
  float min_bound[3] = { 0.f, 0.f, 0.f };
  float max_bound[3] = { 0.f, 0.f, 0.f };
  bool is_infinite = false;

  assert(instance && position && extend_x && extend_y && extend_z);

  APP(get_model_aabb(instance->model, min_bound, max_bound));
  is_infinite =
      (min_bound[0] == -FLT_MAX)
    | (min_bound[1] == -FLT_MAX)
    | (min_bound[2] == -FLT_MAX)
    | (max_bound[0] == FLT_MAX)
    | (max_bound[1] == FLT_MAX)
    | (max_bound[2] == FLT_MAX);

  if(is_infinite) {
    *position = vf4_zero();
    *extend_x = *extend_y = *extend_z = vf4_set1(FLT_MAX);
  } else {
    const struct aosf33 f33 = {
      .c0 = instance->transform.c0,
      .c1 = instance->transform.c1,
      .c2 = instance->transform.c2
    };
    const vf4_t vmin = vf4_set(min_bound[0], min_bound[1], min_bound[2], 1.f);
    const vf4_t vmax = vf4_set(max_bound[0], max_bound[1], max_bound[2], 1.f);
    const vf4_t vhmin = vf4_mul(vmin, vf4_set(0.5f, 0.5f, 0.5f, 1.f));
    const vf4_t vhmax = vf4_mul(vmax, vf4_set(0.5f, 0.5f, 0.5f, 1.f));
    const vf4_t vpos = vf4_add(vhmax, vhmin);
    const vf4_t vext = vf4_sub(vmax, vpos);
    const vf4_t vextx = vf4_and(vext, vf4_mask(true, false, false, true));
    const vf4_t vexty = vf4_and(vext, vf4_mask(false, true, false, true));
    const vf4_t vextz = vf4_and(vext, vf4_mask(false, false, true, true));

    *position = vf4_add(aosf33_mulf3(&f33, vpos), instance->transform.c3);
    *extend_x = aosf33_mulf3(&f33, vextx);
    *extend_y = aosf33_mulf3(&f33, vexty);
    *extend_z = aosf33_mulf3(&f33, vextz);
  }
  return is_infinite;
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
enum app_error
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

enum app_error
app_model_instance_ref_get(struct app_model_instance* instance)
{
  if(!instance)
    return APP_INVALID_ARGUMENT;
  ref_get(&instance->ref);
  return APP_NO_ERROR;
}

enum app_error
app_model_instance_ref_put(struct app_model_instance* instance)
{
  if(!instance)
    return APP_INVALID_ARGUMENT;
  ref_put(&instance->ref, release_model_instance);
  return APP_NO_ERROR;
}

enum app_error
app_model_instance_get_model
  (struct app_model_instance* instance,
   struct app_model** out_model)
{
  if(!instance || !out_model)
    return APP_INVALID_ARGUMENT;
  *out_model = instance->model;
  return APP_NO_ERROR;
}

enum app_error
app_get_raw_model_instance_transform
  (const struct app_model_instance* instance,
   const struct aosf44** transform)
{
  if(!instance || !transform)
    return APP_INVALID_ARGUMENT;
  *transform = &instance->transform;
  return APP_NO_ERROR;
}

enum app_error
app_model_instance_name
  (const struct app_model_instance* instance,
   const char** name)
{
  if(!instance)
    return APP_INVALID_ARGUMENT;
  return app_object_name(instance->app, &instance->obj, name);
}

enum app_error
app_set_model_instance_name
  (struct app_model_instance* instance,
   const char* name)
{
  if(!instance)
    return APP_INVALID_ARGUMENT;
  return app_set_object_name(instance->app, &instance->obj, name);
}

enum app_error
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

enum app_error
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
      struct aosf33 res;
      struct app_model_instance* instance = instance_list[i];
      struct rdr_model_instance** render_instance_list = NULL;
      size_t nb_render_instances = 0;
      const struct aosf33 tmp = {
        .c0 = instance->transform.c0,
        .c1 = instance->transform.c1,
        .c2 = instance->transform.c2
      };
      /* Rotate the instance. */
      aosf33_mulf33(&res, &tmp, &f33);
      instance->transform.c0 = res.c0;
      instance->transform.c1 = res.c1;
      instance->transform.c2 = res.c2;
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

enum app_error
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

enum app_error
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

enum app_error
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

enum app_error
app_model_instance_world
  (struct app_model_instance* instance,
   struct app_world** world)
{
  if(!instance || !world)
    return APP_INVALID_ARGUMENT;
  *world = instance->world;
  return APP_NO_ERROR;
}

enum app_error
app_get_model_instance_aabb
  (const struct app_model_instance* instance,
   float min_bound[3],
   float max_bound[3])
{
  vf4_t vpos, vx, vy, vz;
  bool infinite_box = false;

  if(UNLIKELY(!instance || !min_bound || !max_bound))
    return APP_INVALID_ARGUMENT;

  infinite_box = get_model_instance_obb(instance, &vpos, &vx, &vy, &vz);
  if(infinite_box) {
    min_bound[0] = min_bound[1] = min_bound[2] = -FLT_MAX;
    max_bound[0] = max_bound[1] = max_bound[2] = FLT_MAX;
  } else {
    vf4_t vmin, vmax;
    ALIGN(16) float tmp[4];

    vmin = vmax = vf4_add(vf4_add(vf4_add(vpos, vx), vy), vz);
    #define NEG sub
    #define POS add
    #define UPDATE_BOUND(x, y, z) \
      do { \
        const vf4_t vtmp = \
          CONCAT(vf4_, z)(CONCAT(vf4_, y)(CONCAT(vf4_, x)(vpos, vx), vy), vz); \
        vmin = vf4_min(vmin, vtmp); \
        vmax = vf4_max(vmax, vtmp); \
      } while(0)
    UPDATE_BOUND(NEG, POS, POS);
    UPDATE_BOUND(POS, NEG, POS);
    UPDATE_BOUND(NEG, NEG, POS);
    UPDATE_BOUND(POS, POS, NEG);
    UPDATE_BOUND(NEG, POS, NEG);
    UPDATE_BOUND(POS, NEG, NEG);
    UPDATE_BOUND(NEG, NEG, NEG);
    #undef NEG
    #undef POS
    #undef UPDATE_BOUND

    vf4_store(tmp, vmin);
    min_bound[0] = tmp[0];
    min_bound[1] = tmp[1];
    min_bound[2] = tmp[2];
    vf4_store(tmp, vmax);
    max_bound[0] = tmp[0];
    max_bound[1] = tmp[1];
    max_bound[2] = tmp[2];
  }
  return APP_NO_ERROR;
}

enum app_error
app_get_model_instance_obb
  (const struct app_model_instance* instance,
   float position[3],
   float extend_x[3],
   float extend_y[3],
   float extend_z[3])
{
  vf4_t vpos, vx, vy, vz;
  ALIGN(16) float tmp[4];

  if(UNLIKELY(!instance || !position || !extend_x || !extend_y || !extend_z))
    return APP_INVALID_ARGUMENT;

  get_model_instance_obb(instance, &vpos, &vx, &vy, &vz);
  vf4_store(tmp, vpos);
  position[0] = tmp[0];
  position[1] = tmp[1];
  position[2] = tmp[2];
  vf4_store(tmp, vx);
  extend_x[0] = tmp[0];
  extend_x[1] = tmp[1];
  extend_x[2] = tmp[2];
  vf4_store(tmp, vy);
  extend_y[0] = tmp[0];
  extend_y[1] = tmp[1];
  extend_y[2] = tmp[2];
  vf4_store(tmp, vz);
  extend_z[0] = tmp[0];
  extend_z[1] = tmp[1];
  extend_z[2] = tmp[2];
  return APP_NO_ERROR;
}

enum app_error
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

enum app_error
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

enum app_error
app_get_model_instance_list_length(struct app* app, size_t* len)
{
  if(UNLIKELY(!app || !len))
    return APP_INVALID_ARGUMENT;
  APP(get_object_list(app, APP_MODEL_INSTANCE, len, NULL));
  return APP_NO_ERROR;
}

enum app_error
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

enum app_error
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

