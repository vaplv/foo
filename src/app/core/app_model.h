#ifndef APP_MODEL_H
#define APP_MODEL_H

#include "app/core/app_error.h"

struct app;
struct app_model;

extern enum app_error
app_create_model
  (struct app* app,
   const char* path, /* May be null. */
   struct app_model** model);

extern enum app_error
app_model_ref_put
  (struct app_model* model);

extern enum app_error
app_model_ref_get
  (struct app_model* model);

extern enum app_error
app_clear_model
  (struct app_model* model);

extern enum app_error
app_load_model
  (const char* path,
   struct app_model* model);

extern enum app_error
app_model_name
  (struct app_model* model,
   const char* name);

extern enum app_error
app_get_model_name
  (const struct app_model* model,
   const char** name);

#endif /* APP_MODEL_H */

