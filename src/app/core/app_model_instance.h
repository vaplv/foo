#ifndef APP_MODEL_INSTANCE_H
#define APP_MODEL_INSTANCE_H

#include "app/core/app_error.h"

struct app;
struct app_model;
struct app_model_instance;

extern enum app_error
app_instantiate_model
  (struct app* app,
   struct app_model* model,
   struct app_model_instance** instance);

extern enum app_error
app_free_model_instance
  (struct app* app,
   struct app_model_instance* instance);

#endif /* APP_MODEL_INSTANCE_H */

