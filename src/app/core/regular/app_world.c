#include "app/core/regular/app_core_c.h"
#include "app/core/regular/app_cvar_c.h"
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
#include "stdlib/sl_hash_table.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct app_world {
  struct ref ref;
  struct list_node instance_list; /* Linked list of app_model_instance. */
  struct sl_hash_table* picking_htbl;
  struct app* app;
  struct rdr_world* render_world;
  bool is_picking_setuped;
  uint32_t max_pick_id;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static size_t
hash_pick_id(const void* ptr)
{
  return sl_hash(ptr, sizeof(uint32_t));
}

static bool
eq_pick_id(const void* ptr0, const void* ptr1)
{
  const uint32_t id0 = *(uint32_t*)ptr0;
  const uint32_t id1 = *(uint32_t*)ptr1;
  return id0 == id1;
}

static enum app_error
setup_picking(struct app_world* world)
{
  enum app_error app_err = APP_NO_ERROR;

  if(world->is_picking_setuped == false) {
    struct list_node* node = NULL;
    uint32_t pick_id = 0;
    enum sl_error sl_err = SL_NO_ERROR;

    /* Setup the pick id of the instances. */
    LIST_FOR_EACH(node, &world->instance_list) {
      struct app_model_instance* instance =
        CONTAINER_OF(node, struct app_model_instance, world_node);

      if(pick_id > APP_PICK_ID_MAX) {
        app_err = APP_OVERFLOW_ERROR;
        goto error;
      }
      pick_id = APP_PICK(pick_id, APP_PICK_GROUP_WORLD);
      APP(set_model_instance_pick_id(instance, pick_id));

      sl_err = sl_hash_table_insert(world->picking_htbl, &pick_id, &instance);
      if(sl_err != SL_NO_ERROR) {
        app_err = sl_to_app_error(sl_err);
        goto error;
      }
      ++pick_id;
    }
    world->is_picking_setuped = true;
    world->max_pick_id = pick_id - 1;
  }
exit:
  return app_err;
error:
  SL(hash_table_clear(world->picking_htbl));
  world->is_picking_setuped = false;
  world->max_pick_id = 0;
  goto exit;
}

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
  if(world->picking_htbl)
    SL(free_hash_table(world->picking_htbl));
  MEM_FREE(world->app->allocator, world);
}

/*******************************************************************************
 *
 * Implementation of the app_world functions.
 *
 ******************************************************************************/
enum app_error
app_create_world(struct app* app, struct app_world** out_world)
{
  struct app_world* world = NULL;
  enum app_error err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

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
  sl_err = sl_create_hash_table
    (sizeof(uint32_t),
     ALIGNOF(uint32_t),
     sizeof(void*),
     ALIGNOF(void*),
     hash_pick_id,
     eq_pick_id,
     app->allocator,
     &world->picking_htbl);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_app_error(sl_err);
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

enum app_error
app_world_ref_get(struct app_world* world)
{
  if(!world)
    return APP_INVALID_ARGUMENT;
  ref_get(&world->ref);
  return APP_NO_ERROR;
}

enum app_error
app_world_ref_put(struct app_world* world)
{
  if(!world)
    return APP_INVALID_ARGUMENT;
  ref_put(&world->ref, release_world);
  return APP_NO_ERROR;
}

enum app_error
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

  world->is_picking_setuped = false; /* Reset picking */

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

enum app_error
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

  world->is_picking_setuped = false; /* reset picking */

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

enum app_error
app_world_pick
  (struct app_world* world,
   const struct app_view* view,
   const unsigned int pos[2],
   const unsigned int size[2])
{
  struct rdr_view render_view;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(!world || !view || !pos || !size)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  /* Setup the pick id of the instances. */
  app_err = setup_picking(world);
  if(app_err != APP_NO_ERROR)
    goto error;

  APP(to_rdr_view(world->app, view, &render_view));
  rdr_err = rdr_frame_pick_model_instance
    (world->app->rdr.frame,
     world->render_world,
     &render_view,
     pos,
     size);
  if(rdr_err != RDR_NO_ERROR) {
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }

exit:
  return app_err;
error:
  goto exit;
}

enum app_error
app_world_picked_model_instance
  (struct app_world* world,
   const uint32_t pick_id,
   struct app_model_instance** out_instance)
{
  struct app_model_instance** instance = NULL;
  enum app_error app_err = APP_NO_ERROR;

  if(UNLIKELY
  (  !world
  || !out_instance
  || !world->is_picking_setuped
  || world->max_pick_id < pick_id)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  /* Look for the instance corresponding to the pick_id */
  SL(hash_table_find
    (world->picking_htbl, (void*)&pick_id, (void**)&instance));
  assert(instance != NULL);
  *out_instance = *instance;

exit:
  return app_err;
error:
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
  struct rdr_view render_view;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!world || !view) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  APP(to_rdr_view(world->app, view, &render_view));

  if(world->app->cvar_system.rdr_show_picking->value.boolean == false) {
    rdr_err = rdr_frame_draw_world
      (world->app->rdr.frame, world->render_world, &render_view);
    if(rdr_err != RDR_NO_ERROR) {
      app_err = rdr_to_app_error(rdr_err);
      goto error;
    }
  } else {
    setup_picking(world);
    rdr_err = rdr_frame_show_pick_buffer
      (world->app->rdr.frame,
       world->render_world,
       &render_view,
       world->max_pick_id);
    if(rdr_err != RDR_NO_ERROR) {
      app_err = rdr_to_app_error(rdr_err);
      goto error;
    }
  }

exit:
  return app_err;
error:
  goto exit;
}

