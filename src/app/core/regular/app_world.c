#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_model_instance_c.h"
#include "app/core/regular/app_view_c.h"
#include "app/core/regular/app_world_c.h"
#include "app/core/app_world.h"
#include "maths/simd/aosf44.h"
#include "renderer/rdr_error.h"
#include "renderer/rdr_world.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "window_manager/wm_window.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct app_world {
  struct rdr_world* render_world;
};

/*******************************************************************************
 *
 * Implementation of the app_world functions.
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_create_world(struct app* app, struct app_world** out_world)
{
  struct app_world* world = NULL;
  enum app_error err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!app || !out_world) {
    err = APP_INVALID_ARGUMENT;
    goto error;
  }

  world = MEM_CALLOC(1, sizeof(struct app_world));
  if(!world) {
    err = APP_MEMORY_ERROR;
    goto error;
  }

  rdr_err = rdr_create_world(app->rdr, &world->render_world);
  if(rdr_err != RDR_NO_ERROR) {
    err = rdr_to_app_error(rdr_err);
    goto error;
  }
  rdr_err = rdr_background_color
    (app->rdr, world->render_world, (float[]){0.1f, 0.1f, 0.1f});
  if(rdr_err != RDR_NO_ERROR) {
    err = rdr_to_app_error(rdr_err);
    goto error;
  }

exit:
  if(out_world)
    *out_world = world;
  return err;

error:
  if(world) {
    if(world->render_world)
      RDR(free_world(app->rdr, world->render_world));
    MEM_FREE(world);
    world = NULL;
  }
  goto exit;
}

EXPORT_SYM enum app_error
app_free_world(struct app* app, struct app_world* world)
{
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!app || !world) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  rdr_err = rdr_free_world(app->rdr, world->render_world);
  if(rdr_err != RDR_NO_ERROR) {
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }
  MEM_FREE(world);

exit:
  return app_err;

error:
  goto exit;
}

EXPORT_SYM enum app_error
app_world_add_model_instances
  (struct app* app,
   struct app_world* world,
   size_t nb_model_instances,
   struct app_model_instance* instance_list[])
{
  struct rdr_model_instance** buffer = NULL;
  size_t nb_added_instances = 0;
  size_t len = 0;
  size_t i = 0;
  size_t j = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!app || !world || (nb_model_instances && !instance_list)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  for(i = 0; i < nb_model_instances; ++i) {
    SL(vector_buffer
       (instance_list[i]->model_instance_list,
        &len,
        NULL,
        NULL,
        (void**)&buffer));
    for(j = 0; j < len; ++j) {
      rdr_err = rdr_add_model_instance
        (app->rdr, world->render_world, buffer[j]);
      if(rdr_err != RDR_NO_ERROR) {
        app_err = rdr_to_app_error(rdr_err);
        goto error;
      }
      ++nb_added_instances;
    }
  }

exit:
  return app_err;

error:
  for(i = 0; i < nb_model_instances && nb_added_instances; ++i) {
    SL(vector_buffer
       (instance_list[i]->model_instance_list,
        &len,
        NULL,
        NULL,
        (void**)&buffer));
    for(j = 0; j < len && nb_added_instances; ++j) {
      RDR(remove_model_instance(app->rdr, world->render_world, buffer[i]));
      --nb_added_instances;
    }
  }
  assert(nb_added_instances == 0);
  goto exit;
}

EXPORT_SYM enum app_error
app_world_remove_model_instances
  (struct app* app,
   struct app_world* world,
   size_t nb_model_instances,
   struct app_model_instance* instance_list[])
{
  struct rdr_model_instance** buffer = NULL;
  size_t nb_removed_instances = 0;
  size_t len = 0;
  size_t i = 0;
  size_t j = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app || !world || (nb_model_instances && !instance_list)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  for(i = 0; i < nb_model_instances; ++i) {
    sl_err = sl_vector_buffer
      (instance_list[i]->model_instance_list,
       &len,
       NULL,
       NULL,
       (void**)&buffer);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    for(j = 0; j < len; ++j) {
      rdr_err = rdr_remove_model_instance
        (app->rdr, world->render_world, buffer[j]);
      if(rdr_err != RDR_NO_ERROR) {
        app_err = rdr_to_app_error(rdr_err);
        goto error;
      }
      ++nb_removed_instances;
    }
  }

exit:
  return app_err;

error:
  for(i = 0; i < nb_model_instances && nb_removed_instances; ++i) {
    SL(vector_buffer
       (instance_list[i]->model_instance_list,
        &len,
        NULL,
        NULL,
        (void**)&buffer));
    for(j = 0; j < len && nb_removed_instances; ++j) {
      RDR(add_model_instance(app->rdr, world->render_world, buffer[i]));
      --nb_removed_instances;
    }
  }
  assert(nb_removed_instances == 0);
  goto exit;
}

/*******************************************************************************
 *
 * Private world functions.
 *
 ******************************************************************************/
enum app_error
app_draw_world
  (struct app* app,
   struct app_world* world,
   const struct app_view* view)
{
  struct wm_window_desc win_desc;
  struct rdr_view render_view;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!app || !world || !view) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  WM(get_window_desc(app->wm, app->window, &win_desc));

  assert(sizeof(view->transform) == sizeof(render_view.transform));

  aosf44_store(render_view.transform, &view->transform);
  render_view.proj_ratio = view->ratio;
  render_view.fov_x = view->fov_x;
  render_view.znear = view->znear;
  render_view.zfar = view->zfar;
  render_view.x = 0;
  render_view.y = 0;
  render_view.width = win_desc.width;
  render_view.height = win_desc.height;

  rdr_err = rdr_draw_world(app->rdr, world->render_world, &render_view);
  if(rdr_err != RDR_NO_ERROR) {
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }

exit:
  return app_err;
error:
  goto exit;
}

