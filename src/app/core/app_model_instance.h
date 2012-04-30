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
   const char* name, /* May be NULL. */
   struct app_model_instance** instance); /* May be NULL. */

extern enum app_error
app_model_instance_get_model
  (struct app_model_instance* instance,
   struct app_model** out_model);

extern enum app_error
app_model_instance_ref_put
   (struct app_model_instance* instance);

extern enum app_error
app_model_instance_ref_get
   (struct app_model_instance* instance);

extern enum app_error
app_set_model_instance_name
  (struct app_model_instance* instance,
   const char* name);

extern enum app_error
app_model_instance_name
  (const struct app_model_instance* instance,
   const char** cstr);

extern enum app_error
app_translate_model_instance
  (struct app_model_instance* instance,
   bool local_translation,
   const float translation[3]);

extern enum app_error
app_get_model_instance
  (struct app* app,
   const char* name,
   struct app_model_instance** instance);

extern enum app_error
app_model_instance_name_completion
  (struct app* app,
   const char* instance_name,
   size_t instance_name_len,
   size_t* completion_list_len,
   const char** completion_list[]);

#endif /* APP_MODEL_INSTANCE_H */

