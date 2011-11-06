#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_model_instance_c.h"
#include "app/core/app_model_instance.h"
#include "renderer/rdr_model_instance.h"
#include "stdlib/sl_vector.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>

/*******************************************************************************
 *
 * Implementation of the public model instance functions.
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_free_model_instance(struct app* app, struct app_model_instance* instance)
{
  struct rdr_model_instance** render_instance_lstbuf = NULL;
  size_t len = 0;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;

  if(!app || !instance) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
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
  free(instance);

exit:
  return app_err;

error:
  goto exit;
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
  instance = calloc(1, sizeof(struct app_model_instance));
  if(!instance) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  sl_err = sl_create_vector
    (sizeof(struct app_model_instance*),
     ALIGNOF(struct app_model_instance*),
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
  if(app && instance) {
    app_err = app_free_model_instance(app, instance);
    assert(app_err == APP_NO_ERROR);
  }
  goto exit;
}

