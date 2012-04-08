#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_model_instance_c.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "renderer/rdr.h"
#include "renderer/rdr_model_instance.h"
#include "stdlib/sl.h"
#include "stdlib/sl_set.h"
#include "stdlib/sl_string.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static int
compare_model_instances(const void* a, const void* b)
{
  const struct app_model_instance* i0 = *(const struct app_model_instance**)a;
  const struct app_model_instance* i1 = *(const struct app_model_instance**)b;
  const char* cstr0 = NULL;
  const char* cstr1 = NULL;
  SL(string_get(i0->name, &cstr0));
  SL(string_get(i1->name, &cstr1));
  assert(cstr0 && cstr1);
  return strcmp(cstr0, cstr1);
}

static void
release_model_instance(struct ref* ref)
{
  struct app_model_instance* instance = CONTAINER_OF
    (ref, struct app_model_instance, ref);
  struct app* app = instance->app;
  struct rdr_model_instance** render_instance_lstbuf = NULL;
  size_t len = 0;
  size_t i = 0;
  bool is_registered = false;
  assert(app != NULL);

  APP(is_object_registered
    (instance->app, APP_MODEL_INSTANCE, instance, &is_registered));
  if(is_registered) {
    APP(invoke_callbacks
      (instance->app, APP_SIGNAL_DESTROY_MODEL_INSTANCE, instance));
    APP(unregister_object(instance->app, APP_MODEL_INSTANCE, instance));
  }

 if(instance->model_instance_list) {
    SL(vector_buffer
       (instance->model_instance_list,
        &len,
        NULL,
        NULL,
        (void**)&render_instance_lstbuf));
    for(i = 0; i < len; ++i)
      RDR(model_instance_ref_put(render_instance_lstbuf[i]));
    SL(free_vector(instance->model_instance_list));
  }
  if(instance->name)
    SL(free_string(instance->name));

  APP(model_ref_put(instance->model));
  MEM_FREE(app->allocator, instance);
  APP(ref_put(app));
}

/*******************************************************************************
 *
 * Implementation of the public model instance functions.
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_model_instance_ref_get(struct app_model_instance* instance)
{
  if(!instance)
    return APP_INVALID_ARGUMENT;
  ref_get(&instance->ref);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_model_instance_ref_put(struct app_model_instance* instance)
{
  if(!instance)
    return APP_INVALID_ARGUMENT;
  ref_put(&instance->ref, release_model_instance);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_model_instance_get_model
  (struct app_model_instance* instance,
   struct app_model** out_model)
{
  if(!instance || !out_model)
    return APP_INVALID_ARGUMENT;
  *out_model = instance->model;
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_get_model_instance_name
  (const struct app_model_instance* instance, 
   const char** cstr)
{
  if(!instance || !cstr)
    return APP_INVALID_ARGUMENT;
  SL(string_get(instance->name, cstr));
  return APP_NO_ERROR;
}

/*******************************************************************************
 *
 * Private model instance functions.
 *
 ******************************************************************************/
extern enum app_error
app_create_model_instance
  (struct app* app,
   struct app_model_instance** out_instance)
{
  struct app_model_instance* instance = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app || !out_instance) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  instance = MEM_CALLOC
    (app->allocator, 1, sizeof(struct app_model_instance));
  if(!instance) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  ref_init(&instance->ref);
  APP(ref_get(app));
  instance->app = app;
  sl_err = sl_create_vector
    (sizeof(struct rdr_model_instance*),
     ALIGNOF(struct rdr_model_instance*),
     app->allocator,
     &instance->model_instance_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_create_string(NULL, app->allocator, &instance->name);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
exit:
  if(out_instance)
    *out_instance = instance;
  return app_err;
error:
  if(instance) {
    while(!ref_put(&instance->ref, release_model_instance));
    instance = NULL;
  }
  goto exit;
}

/*******************************************************************************
 *
 * Private app functions.
 *
 ******************************************************************************/
enum app_error
app_setup_model_instance_set(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_create_set
    (sizeof(struct app_model_instance*),
     ALIGNOF(struct app_model_instance*),
     compare_model_instances,
     app->allocator,
     &app->object_list[APP_MODEL_INSTANCE]);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
exit:
  return app_err;
error:
  goto exit;
}

