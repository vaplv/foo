#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_model_instance_c.h"
#include "app/core/app_model_instance.h"
#include "renderer/rdr_model_instance.h"
#include "stdlib/sl_sorted_vector.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>

/*******************************************************************************
 *
 * Private functions.
 *
 ******************************************************************************/
static void
release_model_instance(struct ref* ref) 
{
  struct app_model_instance* instance = CONTAINER_OF
    (ref, struct app_model_instance, ref);
  struct app* app = instance->app;
  struct rdr_model_instance** render_instance_lstbuf = NULL;
  size_t len = 0;
  size_t i = 0;
  bool was_registered = false;

  assert(app != NULL);

 if(instance->model_instance_list) {
    SL(vector_buffer
       (instance->model_instance_list,
        &len,
        NULL,
        NULL,
        (void**)&render_instance_lstbuf));
    for(i = 0; i < len; ++i)
      RDR(free_model_instance(app->rdr, render_instance_lstbuf[i]));
    SL(free_vector(instance->model_instance_list));
  }
  APP(is_model_instance_registered(instance, &was_registered));
  if(was_registered) {
    APP(unregister_model_instance(instance));
  }
  MEM_FREE(app->allocator, instance);
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

exit:
  if(out_instance)
    *out_instance = instance;
  return app_err;

error:
  if(instance)
    while(!ref_put(&instance->ref, release_model_instance));
  goto exit;
}

enum app_error
app_register_model_instance(struct app_model_instance* instance)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  
  if(!instance) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_sorted_vector_insert
    (instance->app->model_instance_list, &instance);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }

exit:
  return app_err;
error:
  goto exit;
}

enum app_error
app_unregister_model_instance(struct app_model_instance* instance)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!instance) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_sorted_vector_remove
    (instance->app->model_instance_list, &instance);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }

exit:
  return app_err;
error:
  goto exit;
}

enum app_error
app_is_model_instance_registered
  (struct app_model_instance* instance,
   bool* is_registered)
{
  enum app_error app_err = APP_NO_ERROR;
  size_t i = 0;
  size_t len = 0;

  if(!instance || !is_registered) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(sorted_vector_find(instance->app->model_instance_list, &instance, &i));
  SL(sorted_vector_buffer
    (instance->app->model_instance_list, &len, NULL, NULL, NULL));
  *is_registered = (i != len);

exit:
  return app_err;
error:
  goto exit;
}

