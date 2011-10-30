#include "maths/simd/aosf44.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_model_instance_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr_world.h"
#include "stdlib/sl_sorted_vector.h"
#include "sys/sys.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct rdr_world {
  float bkg_color[4];
  struct sl_sorted_vector* model_instance_list;
};

/*******************************************************************************
 *
 * Helper functions
 *
 ******************************************************************************/
static void
compute_proj
  (float fov_x, 
   float ratio, 
   float znear, 
   float zfar, 
   struct aosf44* proj)
{
  const float fov_y = fov_x / ratio;
  const float f = cosf(fov_y*0.5f) / sinf(fov_y*0.5f);

  assert(proj);

  proj->c0 = vf4_set(f / ratio, 0.f, 0.f, 0.f);
  proj->c1 = vf4_set(0.f, f, 0.f, 0.f);
  proj->c2 = vf4_set(0.f, 0.f, (zfar + znear) / (znear - zfar), -1.f);
  proj->c3 = vf4_set(0.f, 0.f, (2.f * zfar * znear) / (znear - zfar), 0.f);
}

static int
compare_model_instance(const void* inst0, const void* inst1)
{
  const uintptr_t a = (uintptr_t)(*(struct rdr_model_instance**)inst0);
  const uintptr_t b = (uintptr_t)(*(struct rdr_model_instance**)inst1);
  return -(a < b) | (a > b);
}

/*******************************************************************************
 *
 * Implementation of the render world functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rdr_error
rdr_create_world(struct rdr_system* sys, struct rdr_world** out_world)
{
  struct rdr_world* world = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !out_world) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  world = calloc(1, sizeof(struct rdr_world));
  if(world == NULL) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }

  sl_err = sl_create_sorted_vector
    (sys->sl_ctxt,
     sizeof(struct rdr_model_instance*),
     ALIGNOF(struct rdr_model_instance*),
     compare_model_instance,
     &world->model_instance_list);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

exit:
  if(out_world)
    *out_world = world;
  return rdr_err;

error:
  if(world) {
    if(world->model_instance_list) {
      sl_err = sl_free_sorted_vector(sys->sl_ctxt, world->model_instance_list);
      assert(sl_err == SL_NO_ERROR);
    }
    free(world);
    world = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_free_world(struct rdr_system* sys, struct rdr_world* world)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !world) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  if(world->model_instance_list) {
    struct rdr_model_instance** instance_list = NULL;
    size_t nb_instances = 0;
    size_t i = 0;

    sl_err = sl_sorted_vector_buffer
      (sys->sl_ctxt,
       world->model_instance_list,
       &nb_instances,
       NULL,
       NULL,
       (void**)&instance_list);

    for(i = 0; i < nb_instances; ++i)
      RDR_RELEASE_OBJECT(sys, instance_list[i]);

    sl_err = sl_free_sorted_vector(sys->sl_ctxt, world->model_instance_list);
    if(sl_err != SL_NO_ERROR) {
      rdr_err = sl_to_rdr_error(sl_err);
      goto error;
    }
  }
  free(world);

exit:
  return rdr_err;

error:
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_add_model_instance
  (struct rdr_system* sys,
   struct rdr_world* world,
   struct rdr_model_instance* instance)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool is_instance_added = false;
  bool is_instance_retained = false;

  if(!sys || !world || !instance) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  sl_err = sl_sorted_vector_insert
    (sys->sl_ctxt, world->model_instance_list, &instance);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }
  is_instance_added = true;

  rdr_err = RDR_RETAIN_OBJECT(sys, instance);
  if(rdr_err != RDR_NO_ERROR)
    goto error;
  is_instance_retained = true;

exit:
  return rdr_err;

error:
  if(is_instance_added) {
    assert(world);
    sl_err = sl_sorted_vector_remove
      (sys->sl_ctxt, world->model_instance_list, &instance);
    assert(sl_err == SL_NO_ERROR);
  }
  if(is_instance_retained)
    RDR_RELEASE_OBJECT(sys, instance);
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_remove_model_instance
  (struct rdr_system* sys,
   struct rdr_world* world,
   struct rdr_model_instance* instance)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool is_instance_removed = false;
  bool is_instance_released = false;

  if(!sys || !world || !instance) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  sl_err = sl_sorted_vector_remove
    (sys->sl_ctxt, world->model_instance_list, &instance);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }
  is_instance_removed = true;

  RDR_RELEASE_OBJECT(sys, instance);
  is_instance_released = true;

exit:
  return rdr_err;

error:
  if(is_instance_removed) {
    assert(world);
    sl_err = sl_sorted_vector_insert
      (sys->sl_ctxt, world->model_instance_list, &instance);
    assert(sl_err == SL_NO_ERROR);
  }
  if(is_instance_released)
    RDR_RETAIN_OBJECT(sys, instance);
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_draw_world
  (struct rdr_system* sys,
   struct rdr_world* world,
   const struct rdr_view* view)
{
  const struct rb_depth_stencil_desc depth_stencil_desc = {
    .enable_depth_test = 1,
    .enable_stencil_test = 0,
    .depth_func = RB_COMPARISON_LESS_EQUAL,
  };
  struct aosf44 view_matrix;
  struct aosf44 proj_matrix;
  struct rdr_model_instance** instance_list = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  size_t nb_instances = 0;
  int err = 0;

  if(!sys || !world || !view) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  err = sys->rb.clear
    (sys->ctxt,
     RB_CLEAR_COLOR_BIT | RB_CLEAR_DEPTH_BIT | RB_CLEAR_STENCIL_BIT,
     world->bkg_color,
     1.f,
     0);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

  err = sys->rb.depth_stencil(sys->ctxt, &depth_stencil_desc);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

  sl_err = sl_sorted_vector_buffer
    (sys->sl_ctxt,
     world->model_instance_list,
     &nb_instances,
     NULL,
     NULL,
     (void**)&instance_list);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

  assert(instance_list != NULL || nb_instances == 0);
  if(instance_list != NULL) {
    aosf44_load(&view_matrix, view->transform);
    compute_proj
      (view->fov_x, view->proj_ratio, view->znear, view->zfar, &proj_matrix);
    rdr_err = rdr_draw_instances
      (sys, &view_matrix, &proj_matrix, nb_instances, instance_list);
    if(rdr_err != RDR_NO_ERROR)
      goto error;
  }

exit:
  return rdr_err;

error:
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_background_color
  (struct rdr_system* sys,
   struct rdr_world* world,
   const float rgb[3])
{
  if(!sys || !world || !rgb)
    return RDR_INVALID_ARGUMENT;

  memcpy(world->bkg_color, rgb, 3 * sizeof(float));
  return RDR_NO_ERROR;
}

