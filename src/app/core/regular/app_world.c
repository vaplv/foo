#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_model_instance_c.h"
#include "app/core/regular/app_view_c.h"
#include "app/core/regular/app_world_c.h"
#include "app/core/app_cvar.h"
#include "app/core/app_model_instance.h"
#include "app/core/app_world.h"
#include "maths/simd/aosf44.h"
#include "renderer/rdr.h"
#include "renderer/rdr_error.h"
#include "renderer/rdr_frame.h"
#include "renderer/rdr_world.h"
#include "stdlib/sl.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include "window_manager/wm.h"
#include "window_manager/wm_window.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct app_world {
  struct ref ref;
  struct list_node instance_list; /* Linked list of app_model_instance. */
  struct app* app;
  struct rdr_world* render_world;
};

/*******************************************************************************
 *
 * World functions.
 *
 ******************************************************************************/
static void
release_world(struct ref* ref)
{
  struct app_world* world = NULL;
  struct list_node* node = NULL;
  struct list_node* tmp = NULL;
  assert(ref != NULL);

  world = CONTAINER_OF(ref, struct app_world, ref);
  LIST_FOR_EACH_SAFE(node, tmp, &world->instance_list) {
    struct app_model_instance* instance =
      CONTAINER_OF(node, struct app_model_instance, world_node);
    instance->world = NULL;
    list_del(&instance->world_node);
  }
  if(world->render_world)
    RDR(world_ref_put(world->render_world));
  MEM_FREE(world->app->allocator, world);
}

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

  world = MEM_CALLOC(app->allocator, 1, sizeof(struct app_world));
  if(!world) {
    err = APP_MEMORY_ERROR;
    goto error;
  }
  world->app = app;
  ref_init(&world->ref);
  list_init(&world->instance_list);

  rdr_err = rdr_create_world(app->rdr.system, &world->render_world);
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
    APP(world_ref_put(world));
    world = NULL;
  }
  goto exit;
}

EXPORT_SYM enum app_error
app_world_ref_get(struct app_world* world)
{
  if(!world)
    return APP_INVALID_ARGUMENT;
  ref_get(&world->ref);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_world_ref_put(struct app_world* world)
{
  if(!world)
    return APP_INVALID_ARGUMENT;
  ref_put(&world->ref, release_world);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_world_add_model_instances
  (struct app_world* world,
   size_t nb_model_instances,
   struct app_model_instance* instance_list[])
{
  struct rdr_model_instance** buffer = NULL;
  size_t nb_added_rdr_instances = 0;
  size_t nb_added_app_instances = 0;
  size_t len = 0;
  size_t i = 0;
  size_t j = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!world || (nb_model_instances && !instance_list)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  for(i = 0; i < nb_model_instances; ++i) {
    /* Link the instance against the world linked list. */
    if(instance_list[i]->world != NULL) {
      app_err = APP_INVALID_ARGUMENT;
      goto error;
    }
    instance_list[i]->world = world;
    list_add(&world->instance_list, &instance_list[i]->world_node);
    ++nb_added_app_instances;
    /* Add the render data of the instance into the render world. */
    SL(vector_buffer
       (instance_list[i]->model_instance_list,
        &len, NULL, NULL, (void**)&buffer));
    for(j = 0; j < len; ++j) {
      rdr_err = rdr_add_model_instance(world->render_world, buffer[j]);
      if(rdr_err != RDR_NO_ERROR) {
        app_err = rdr_to_app_error(rdr_err);
        goto error;
      }
      ++nb_added_rdr_instances;
    }
  }

exit:
  return app_err;

error:
  for(i = 0; i < nb_model_instances && nb_added_rdr_instances; ++i) {
    SL(vector_buffer
       (instance_list[i]->model_instance_list,
        &len, NULL, NULL, (void**)&buffer));
    for(j = 0; j < len && nb_added_rdr_instances; ++j) {
      RDR(remove_model_instance(world->render_world, buffer[i]));
      --nb_added_rdr_instances;
    }
  }
  for(i = 0; i < nb_added_app_instances; ++i) {
    list_del(&instance_list[i]->world_node);
    instance_list[i]->world = NULL;
  }
  goto exit;
}

EXPORT_SYM enum app_error
app_world_remove_model_instances
  (struct app_world* world,
   size_t nb_model_instances,
   struct app_model_instance* instance_list[])
{
  struct rdr_model_instance** buffer = NULL;
  size_t nb_removed_rdr_instances = 0;
  size_t nb_removed_app_instances = 0;
  size_t len = 0;
  size_t i = 0;
  size_t j = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!world || (nb_model_instances && !instance_list)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  for(i = 0; i < nb_model_instances; ++i) {
    if(instance_list[i]->world != world) {
      app_err = APP_INVALID_ARGUMENT;
      goto error;
    }
    instance_list[i]->world = NULL;
    list_del(&instance_list[i]->world_node);
    ++nb_removed_app_instances;

    sl_err = sl_vector_buffer
      (instance_list[i]->model_instance_list,
       &len, NULL, NULL, (void**)&buffer);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    for(j = 0; j < len; ++j) {
      rdr_err = rdr_remove_model_instance(world->render_world, buffer[j]);
      if(rdr_err != RDR_NO_ERROR) {
        app_err = rdr_to_app_error(rdr_err);
        goto error;
      }
      ++nb_removed_rdr_instances;
    }
  }

exit:
  return app_err;

error:
  for(i = 0; i < nb_model_instances && nb_removed_rdr_instances; ++i) {
    SL(vector_buffer
       (instance_list[i]->model_instance_list,
        &len, NULL, NULL, (void**)&buffer));
    for(j = 0; j < len && nb_removed_rdr_instances; ++j) {
      RDR(add_model_instance(world->render_world, buffer[i]));
      --nb_removed_rdr_instances;
    }
  }
  for(i = 0; i < nb_removed_app_instances; ++i) {
    instance_list[i]->world = world;
    list_add(&world->instance_list, &instance_list[i]->world_node);
  }
  assert(nb_removed_rdr_instances == 0);
  goto exit;
}

/*******************************************************************************
 *
 * Private world functions.
 *
 ******************************************************************************/
enum app_error
app_draw_world(struct app_world* world, const struct app_view* view)
{
  struct wm_window_desc win_desc;
  struct rdr_view render_view;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!world || !view) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  WM(get_window_desc(world->app->wm.window, &win_desc));

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

  rdr_err = rdr_frame_draw_world
    (world->app->rdr.frame, world->render_world, &render_view);
  if(rdr_err != RDR_NO_ERROR) {
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }

  if(world->app->cvar_system.show_aabb->value.boolean) {
    struct list_node* node = NULL;
    LIST_FOR_EACH(node, &world->instance_list) {
      float min_bound[3] = {0.f, 0.f, 0.f};
      float max_bound[3] = {0.f, 0.f, 0.f};
      float size[3] = {0.f, 0.f, 0.f};
      float pos[3] = {0.f, 0.f, 0.f};
      struct app_model_instance* instance = CONTAINER_OF
        (node, struct app_model_instance, world_node);

      APP(get_model_instance_aabb(instance, min_bound, max_bound));
      pos[0] = (min_bound[0] + max_bound[0]) * 0.5f;
      pos[1] = (min_bound[1] + max_bound[1]) * 0.5f;
      pos[2] = (min_bound[2] + max_bound[2]) * 0.5f;
      size[0] = max_bound[0] - min_bound[0];
      size[1] = max_bound[1] - min_bound[1];
      size[2] = max_bound[2] - min_bound[2];
      RDR(frame_imdraw_parallelepiped
        (world->app->rdr.frame,
         &render_view,
         pos,
         size,
         (float[]){0.f, 0.f, 0.f},
         (float[]){0.5f, 0.5f, 0.5f, 0.15f},
         (float[]){0.75f, 0.75f, 0.75f, 1.f}));
    }
  }

exit:
  return app_err;
error:
  goto exit;
}

