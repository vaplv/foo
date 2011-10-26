#ifndef APP_MODEL_INSTANCE_C_H
#define APP_MODEL_INSTANCE_C_H

#include "app/core/app_error.h"

struct sl_vector;

struct app_model_instance {
  struct sl_vector* model_instance_list; /* list of rdr_model_instance*. */
};

extern enum app_error
app_create_model_instance
  (struct app* app,
   struct app_model_instance** instance);

#endif /* APP_MODEL_INSTANCE_C_H. */

