#ifndef APP_WORLD_H
#define APP_WORLD_H

#include "app/core/app.h"
#include "app/core/app_error.h"
#include <stddef.h>

struct app;
struct app_model;
struct app_model_instance;
struct app_world;

APP_API enum app_error
app_create_world
  (struct app* app,
   struct app_world** out_world);

APP_API enum app_error
app_world_ref_get
  (struct app_world* world);

APP_API enum app_error
app_world_ref_put
  (struct app_world* world);

APP_API enum app_error
app_world_add_model_instances
  (struct app_world* world,
   size_t nb_model_instances,
   struct app_model_instance* instance_list[]);

APP_API enum app_error
app_world_remove_model_instances
  (struct app_world* world,
   size_t nb_model_instances,
   struct app_model_instance* instance_list[]);

APP_API enum app_error
app_world_pick
  (struct app_world* world,
   const struct app_view* view,
   const unsigned int pos[2],
   const unsigned int size[2]);

/* Return the model instance corresponding to the pick_id generally retrieved
 * from the app_ppoll_picking function */
APP_API enum app_error
app_world_picked_model_instance
  (struct app_world* world,
   const uint32_t pich_id,
   struct app_model_instance** instance);

#endif /* APP_WORLD_H */

