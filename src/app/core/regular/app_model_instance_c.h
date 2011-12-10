#ifndef APP_MODEL_INSTANCE_C_H
#define APP_MODEL_INSTANCE_C_H

#include "app/core/app_error.h"
#include <stdbool.h>

struct app_model_instance;
struct sl_vector;

/* Data of app_model_instance. */
struct app_model_instance {
  struct app* app;
  struct sl_vector* model_instance_list; /* list of rdr_model_instance*. */
  struct ref ref;
};

extern enum app_error
app_create_model_instance
  (struct app* app,
   struct app_model_instance** instance);

extern enum app_error
app_is_model_instance_registered
  (struct  app_model_instance* instance,
   bool* is_registered);

#endif /* APP_MODEL_INSTANCE_C_H. */

