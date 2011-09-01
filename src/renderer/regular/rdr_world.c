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
compute_proj(float fov_x, float ratio, float znear, float zfar, float* proj)
{
  const float fov_y = fov_x / ratio;
  const float f = cosf(fov_y*0.5f) / sinf(fov_y*0.5f);
  proj[0] = f / ratio;
  proj[1] = 0.f;
  proj[2] = 0.f;
  proj[3] = 0.f;

  proj[4] = 0.f;
  proj[5] = f;
  proj[6] = 0.f;
  proj[7] = 0.f;

  proj[8] = 0.f;
  proj[9] = 0.f;
  proj[10] = (zfar + znear) / (znear - zfar);
  proj[11] = -1.f;

  proj[12] = 0.f;
  proj[13] = 0.f;
  proj[14] = (2.f * zfar *znear) / (znear - zfar);
  proj[15] = 0.f;
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

exit:
  return rdr_err;

error:
  if(is_instance_added) {
    assert(world);
    sl_err = sl_sorted_vector_remove
      (sys->sl_ctxt, world->model_instance_list, &instance);
    assert(sl_err == SL_NO_ERROR);
  }
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

exit:
  return rdr_err;

error:
  if(is_instance_removed) {
    assert(world);
    sl_err = sl_sorted_vector_insert
      (sys->sl_ctxt, world->model_instance_list, &instance);
    assert(sl_err == SL_NO_ERROR);
  }
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
  float proj_matrix[16];
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
    compute_proj
      (view->fov_x, view->proj_ratio, view->znear, view->zfar, proj_matrix);

    rdr_err = rdr_draw_instances
      (sys, view->transform, proj_matrix, nb_instances, instance_list);
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
 
