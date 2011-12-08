#ifndef APP_MODEL_INSTANCE_H
#define APP_MODEL_INSTANCE_H

#include "app/core/app_error.h"
#include "sys/ref_count.h"

struct app;
struct app_model;
struct app_model_instance;

extern enum app_error
app_instantiate_model
  (struct app* app,
   struct app_model* model,
   struct app_model_instance** instance);

extern enum app_error
app_model_instance_ref_put
   (struct app_model_instance* instance);

extern enum app_error
app_model_instance_ref_get
   (struct app_model_instance* instance);

#endif /* APP_MODEL_INSTANCE_H */

