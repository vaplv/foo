#ifndef APP_WORLD_H
#define APP_WORLD_H

#include "app/core/app_error.h"
#include <stddef.h>

struct app;
struct app_model;
struct app_model_instance;
struct app_world;

extern enum app_error
app_create_world
  (struct app* app,
   struct app_world** out_world);

extern enum app_error
app_world_ref_get
  (struct app_world* world);

extern enum app_error
app_world_ref_put
  (struct app_world* world);

extern enum app_error
app_world_add_model_instances
  (struct app_world* world,
   size_t nb_model_instances,
   struct app_model_instance* instance_list[]);

extern enum app_error
app_world_remove_model_instances
  (struct app_world* world,
   size_t nb_model_instances,
   struct app_model_instance* instance_list[]);

#endif /* APP_WORLD_H */

