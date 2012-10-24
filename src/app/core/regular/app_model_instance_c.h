#ifndef APP_MODEL_INSTANCE_C_H
#define APP_MODEL_INSTANCE_C_H

#include "app/core/regular/app_object.h"
#include "app/core/app_error.h"
#include "maths/simd/aosf44.h"
#include "sys/list.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <stdbool.h>

struct app_model_instance;
struct sl_vector;

enum app_model_instance_it_type {
  APP_REGISTERED_MODEL_INSTANCE_IT,
  APP_SPAWNED_MODEL_INSTANCE_IT
};

/* Data of app_model_instance. */
struct app_model_instance {
  ALIGN(16) struct aosf44 transform;
  struct app_object obj;
  struct ref ref;
  struct list_node model_node; /* Linked the instance against its model.*/
  struct list_node world_node; /* Linked the instance against its world. */
  struct app* app;
  struct app_model* model;
  struct app_world* world;
  struct sl_vector* model_instance_list; /* list of rdr_model_instance*. */
  bool invoke_clbk; /* Help for error management. For internal use nly. */
};

LOCAL_SYM enum app_error
app_create_model_instance
  (struct app* app,
   const char* name,
   struct app_model_instance** instance);

LOCAL_SYM enum app_error
app_is_model_instance_registered
  (struct  app_model_instance* instance,
   bool* is_registered);

LOCAL_SYM struct app_model_instance*
app_object_to_model_instance
  (struct app_object* obj);

LOCAL_SYM enum app_error
app_set_model_instance_pick_id
  (struct app_model_instance* instance,
   const uint32_t pick_id);

#endif /* APP_MODEL_INSTANCE_C_H. */

