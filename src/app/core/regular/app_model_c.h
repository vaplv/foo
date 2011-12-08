#ifndef APP_MODEL_C_H
#define APP_MODEL_C_H

#include "app/core/app_error.h"
#include <stdbool.h>

struct app_model;

extern enum app_error
app_register_model
  (struct app_model* model);

extern enum app_error
app_unregister_model
  (struct app_model* model);

extern enum app_error
app_is_model_registered
  (struct app_model* model,
   bool* is_registered);

#endif /* APP_MODEL_C_H */

