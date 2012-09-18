#ifndef APP_MODEL_H
#define APP_MODEL_H

#include "app/core/app.h"
#include "app/core/app_error.h"
#include <stdbool.h>

struct app;
struct app_model;
struct app_model_instance;
struct app_model_spawned_instance_it {
  struct app_model_instance* instance;
  struct app_model* model;
  /* Private data. */
  struct list_node* next;
};
struct app_model_it {
  struct app_model* model;
  /* Private data. */
  size_t id;
};

APP_API enum app_error
app_create_model
  (struct app* app,
   const char* path, /* May be NULL. */
   const char* name, /* May be NULL. */
   struct app_model** model);

/* Remove the model and its associated instance from the application. If the
 * model is referenced elsewehere, it is unregistered from the application
 * without freeing it. */
APP_API enum app_error
app_remove_model
  (struct app_model* model);

APP_API enum app_error
app_model_ref_put
  (struct app_model* model);

APP_API enum app_error
app_model_ref_get
  (struct app_model* model);

APP_API enum app_error
app_clear_model
  (struct app_model* model);

APP_API enum app_error
app_load_model
  (const char* path,
   struct app_model* model);

APP_API enum app_error
app_model_path
  (const struct app_model* model,
   const char** path);

APP_API enum app_error
app_set_model_name
  (struct app_model* model,
   const char* name);

APP_API enum app_error
app_model_name
  (const struct app_model* model,
   const char** name);

APP_API enum app_error
app_is_model_instantiated
  (const struct app_model* model,
   bool* is_instantiated);

APP_API enum app_error
app_get_model_aabb
  (const struct app_model* mdl,
   float min_bound[3],
   float max_bound[3]);

/* Retrieve the iterator onto the list of instances spawned from mdl. */
APP_API enum app_error
app_get_model_spawned_instance_list_begin
  (struct app_model* mdl,
   struct app_model_spawned_instance_it* it,
   bool* is_end_reached);

APP_API enum app_error
app_model_spawned_instance_it_next
  (struct app_model_spawned_instance_it* it,
   bool* is_end_reached);

APP_API enum app_error
app_get_model
  (struct app* app,
   const char* mdl_name,
   struct app_model** mdl); /* Set to NULL if not found. */

APP_API enum app_error
app_get_model_list_begin
  (struct app* app,
   struct app_model_it* it,
   bool* is_end_reached);

APP_API enum app_error
app_model_it_next
  (struct app_model_it* it,
   bool* is_end_reached);

APP_API enum app_error
app_get_model_list_length
  (struct app* app,
   size_t* len);

APP_API enum app_error
app_model_name_completion
  (struct app* app,
   const char* mdl_name,
   size_t mdl_name_len,
   size_t* completion_list_len,
   const char** completion_list[]);

#endif /* APP_MODEL_H */

