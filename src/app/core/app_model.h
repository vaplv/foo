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
app_free_model
  (struct app* app,
   struct app_model* model);

extern enum app_error
app_clear_model
  (struct app* app,
   struct app_model* model);

extern enum app_error
app_load_model
  (struct app* app,
   const char* path,
   struct app_model* model);

#endif /* APP_MODEL_H */

