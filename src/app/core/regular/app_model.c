#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_model_instance_c.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "renderer/rdr.h"
#include "renderer/rdr_material.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_model.h"
#include "renderer/rdr_model_instance.h"
#include "resources/rsrc_geometry.h"
#include "resources/rsrc_wavefront_obj.h"
#include "stdlib/sl.h"
#include "stdlib/sl_string.h"
#include "stdlib/sl_set.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct app_model {
  struct ref ref;
  struct app* app;
  struct sl_string* name;
  struct sl_string* resource_path;
  struct rsrc_geometry* geometry;
  struct sl_vector* mesh_list; /* List of rdr_mesh*. */
  struct sl_vector* material_list; /* list of rdr_material*. */
  struct sl_vector* model_list; /* list of rdr_model*. */
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static int
compare_models(const void* a, const void* b)
{
  const struct app_model* mdl0 = *(const struct app_model**)a;
  const struct app_model* mdl1 = *(const struct app_model**)b;
  const char* cstr0 = NULL;
  const char* cstr1 = NULL;
  SL(string_get(mdl0->name, &cstr0));
  SL(string_get(mdl1->name, &cstr1));
  assert(cstr0 && cstr1);
  return strcmp(cstr0, cstr1);
}

/* Clear the render data of the model, i.e. the material/mesh/model lists. */
static enum app_error
clear_model_render_data(struct app_model* model)
{
  struct rdr_mesh** mesh_lstbuf = NULL;
  struct rdr_material** mtr_lstbuf = NULL;
  struct rdr_model** mdl_lstbuf = NULL;
  size_t i = 0;
  size_t len = 0;

  assert(model);

  if(model->mesh_list) {
    SL(vector_buffer
       (model->mesh_list, &len, NULL, NULL, (void**)&mesh_lstbuf));
    for(i = 0; i < len; ++i)
      RDR(mesh_ref_put(mesh_lstbuf[i]));
    SL(clear_vector(model->mesh_list));
  }
  if(model->material_list) {
    SL(vector_buffer
       (model->material_list, &len, NULL, NULL, (void**)&mtr_lstbuf));
    for(i = 0; i < len; ++i)
      RDR(material_ref_put(mtr_lstbuf[i]));
    SL(clear_vector(model->material_list));
  }
  if(model->model_list) {
    SL(vector_buffer
       (model->model_list, &len, NULL, NULL, (void**)&mdl_lstbuf));
    for(i = 0; i < len; ++i)
      RDR(model_ref_put(mdl_lstbuf[i]));
    SL(clear_vector(model->model_list));
  }
  return APP_NO_ERROR;
}

static enum app_error
setup_model(struct app_model* model)
{
  struct rdr_mesh* mesh = NULL;
  struct rdr_model* rmodel = NULL;
  size_t nb_prim_set = 0;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  assert(model && model->geometry);

  app_err = clear_model_render_data(model);
  if(app_err != APP_NO_ERROR)
    goto error;

  RSRC(get_primitive_set_count(model->geometry, &nb_prim_set));
  for(i = 0; i < nb_prim_set; ++i) {
    struct rsrc_primitive_set prim_set;
    size_t j = 0;
    RSRC(get_primitive_set(model->geometry, i, &prim_set));

    /* Only triangular geometry are handled. */
    if(prim_set.primitive_type != RSRC_TRIANGLE)
      continue;

    /* Render mesh setup. */
    rdr_err = rdr_create_mesh(model->app->rdr.system, &mesh);
    if(rdr_err != RDR_NO_ERROR) {
      app_err = rdr_to_app_error(rdr_err);
      goto error;
    }
    rdr_err = rdr_mesh_indices(mesh, prim_set.nb_indices, prim_set.index_list);
    if(rdr_err != RDR_NO_ERROR) {
      app_err = rdr_to_app_error(rdr_err);
      goto error;
    }
    assert(prim_set.nb_attribs > 0);
    struct rdr_mesh_attrib mesh_attribs[prim_set.nb_attribs];
    for(j = 0; j < prim_set.nb_attribs; ++j) {
      mesh_attribs[j].type = rsrc_to_rdr_type(prim_set.attrib_list[j].type);
      mesh_attribs[j].usage=
        rsrc_to_rdr_attrib_usage(prim_set.attrib_list[j].usage);
    }
    rdr_err = rdr_mesh_data
      (mesh,
       prim_set.nb_attribs,
       mesh_attribs,
       prim_set.sizeof_data,
       prim_set.data);
    if(rdr_err != RDR_NO_ERROR) {
      app_err = rdr_to_app_error(rdr_err);
      goto error;
    }
    /* Render model setup. */
    rdr_err = rdr_create_model
      (model->app->rdr.system,
       mesh,
       model->app->rdr.default_material,
       &rmodel);
    if(rdr_err != RDR_NO_ERROR) {
      app_err = rdr_to_app_error(rdr_err);
      goto error;
    }

    /* Register the mesh and the model. */
    sl_err = sl_vector_push_back(model->mesh_list, &mesh);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    mesh = NULL;
    sl_err = sl_vector_push_back(model->model_list, &rmodel);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    rmodel = NULL;
  }

exit:
  return app_err;

error:
  if(mesh)
    RDR(mesh_ref_put(mesh));
  if(rmodel)
    RDR(model_ref_put(rmodel));
  goto exit;
}

static void
release_model(struct ref* ref)
{
  struct app_model* model = CONTAINER_OF(ref, struct app_model, ref);
  bool is_registered = true;
  assert(ref != NULL);

  APP(is_object_registered(model->app, APP_MODEL, model, &is_registered));
  if(is_registered) {
    APP(invoke_callbacks(model->app, APP_SIGNAL_DESTROY_MODEL, model));
    APP(unregister_object(model->app, APP_MODEL, model));
  }

  APP(clear_model(model));

  if(model->name)
    SL(free_string(model->name));
  if(model->resource_path)
    SL(free_string(model->resource_path));
  if(model->geometry)
    RSRC(geometry_ref_put(model->geometry));
  if(model->mesh_list)
    SL(free_vector(model->mesh_list));
  if(model->material_list)
    SL(free_vector(model->material_list));
  if(model->model_list)
    SL(free_vector(model->model_list));

  MEM_FREE(model->app->allocator, model);
}

/*******************************************************************************
 *
 *  Implementation of the app_model functions.
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_create_model(struct app* app, const char* path, struct app_model** model)
{
  struct app_model* mdl = NULL;
  size_t len = 0;
  size_t i = 0;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  enum app_error app_err = APP_NO_ERROR;
  bool b = false;

  if(!app || !model) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  mdl = MEM_CALLOC(app->allocator, 1, sizeof(struct app_model));
  if(NULL == mdl) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  mdl->app = app;
  ref_init(&mdl->ref);

  #define CALL(func) \
    do { \
      if(SL_NO_ERROR != (sl_err = func)) { \
        app_err = sl_to_app_error(sl_err); \
        goto error; \
      } \
    } while(0)

  CALL(sl_create_string(NULL, app->allocator, &mdl->name));
  CALL(sl_create_string(NULL, app->allocator, &mdl->resource_path));
  rsrc_err = rsrc_create_geometry(app->rsrc.context, &mdl->geometry);
  if(rsrc_err != RSRC_NO_ERROR) {
    app_err = rsrc_to_app_error(rsrc_err);
    goto error;
  }
  CALL(sl_create_vector
    (sizeof(struct rdr_mesh*),
     ALIGNOF(struct rdr_mesh*),
     app->allocator,
     &mdl->mesh_list));
  CALL(sl_create_vector
    (sizeof(struct rdr_material*),
     ALIGNOF(struct rdr_material*),
     app->allocator,
     &mdl->material_list));
  CALL(sl_create_vector
    (sizeof(struct rdr_model*),
     ALIGNOF(struct rdr_model*),
     app->allocator,
     &mdl->model_list));

  if(path) {
    app_err = app_load_model(path, mdl);
    if(app_err != APP_NO_ERROR)
      goto error;
  }

  CALL(sl_string_append(mdl->name, "_0"));
  SL(string_length(mdl->name, &len));
  --len;

  for(i = 0; APP(is_object_registered(app, APP_MODEL, mdl, &b)), b; ++i) {
    char buf[16] = { [15] = '\0' };
    SL(string_erase(mdl->name, len, SIZE_MAX));
    snprintf(buf, 15, "%zu", i);
    CALL(sl_string_append(mdl->name, buf));
  }

  app_err = app_register_object(app, APP_MODEL, mdl);
  if(app_err != APP_NO_ERROR)
    goto error;

  APP(invoke_callbacks(app, APP_SIGNAL_CREATE_MODEL, mdl));

  #undef CALL

exit:
  if(model)
    *model = mdl;
  return app_err;

error:
  if(mdl) {
    APP(model_ref_put(mdl));
    mdl = NULL;
  }
  goto exit;
}

EXPORT_SYM enum app_error
app_clear_model(struct app_model* model)
{
  enum app_error app_err = APP_NO_ERROR;

  if(!model) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  if(model->name) {
    const enum sl_error sl_err = sl_clear_string(model->name);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
  }
  if(model->resource_path) {
    const enum sl_error sl_err = sl_clear_string(model->resource_path);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
  }
  if(model->geometry)
    RSRC(clear_geometry(model->geometry));
  app_err = clear_model_render_data(model);
  if(app_err != APP_NO_ERROR)
    goto error;

exit:
  return app_err;

error:
  goto exit;
}

EXPORT_SYM enum app_error
app_model_ref_get(struct app_model* model)
{
  if(!model)
    return APP_INVALID_ARGUMENT;
  ref_get(&model->ref);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_model_ref_put(struct app_model* model)
{
  if(!model)
    return APP_INVALID_ARGUMENT;
  ref_put(&model->ref, release_model);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_load_model(const char* path, struct app_model* model)
{
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool b = false;

  if(!path || !model) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  assert(model->app && model->app->rsrc.wavefront_obj != NULL);

  rsrc_err = rsrc_load_wavefront_obj
    (model->app->rsrc.wavefront_obj, path);
  if(rsrc_err != RSRC_NO_ERROR) {
    APP_LOG_ERR
      (model->app->logger, "Error loading geometry resource `%s'\n", path);
    app_err = rsrc_to_app_error(rsrc_err);
    goto error;
  }
  rsrc_err = rsrc_geometry_from_wavefront_obj
    (model->geometry, model->app->rsrc.wavefront_obj);
  if(rsrc_err != RSRC_NO_ERROR) {
    app_err = rsrc_to_app_error(rsrc_err);
    goto error;
  }
  app_err = setup_model(model);
  if(app_err != APP_NO_ERROR) {
    goto error;
  }
  sl_err = sl_string_set(model->resource_path, path);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  if(SL(is_string_empty(model->name, &b)), b) {
    sl_err = sl_string_set(model->name, path);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
  }

exit:
  return app_err;
error:
  if(model)
    APP(clear_model(model));
  goto exit;
}

EXPORT_SYM enum app_error
app_get_model_path
  (const struct app_model* model,
   const char** path)
{
  enum app_error app_err = APP_NO_ERROR;
  bool is_empty = false;

  if(!model || !path) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(is_string_empty(model->resource_path, &is_empty));
  if(is_empty) {
    *path = NULL;
  } else {
    SL(string_get(model->resource_path, path));
  }
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_set_model_name(struct app_model* model, const char* name)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool b = false;

  if(!model || !name) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  sl_err = sl_string_set(model->name, name);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }

  /* Re-register the model in order to re-order the app model set with respect
   * to its new name. */
  if(APP(is_object_registered(model->app, APP_MODEL, model, &b)), b) {
    APP(unregister_object(model->app, APP_MODEL, model));
    APP(register_object(model->app, APP_MODEL, model));
  }
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_get_model_name(const struct app_model* model, const char** name)
{
  enum app_error app_err = APP_NO_ERROR;
  bool is_empty = false;

  if(!model || !name) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(is_string_empty(model->name, &is_empty));
  if(is_empty) {
   *name = NULL;
  } else {
    SL(string_get(model->name, name));
  }
exit:
  return app_err;
error:
  goto exit;
}

/*******************************************************************************
 *
 * Model instance function(s).
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_instantiate_model
  (struct app* app,
   struct app_model* model,
   struct app_model_instance** out_instance)
{
  struct app_model_instance* instance = NULL;
  struct rdr_model_instance* render_instance = NULL;
  const char* cstr = NULL;
  size_t len = 0;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool is_ref_get = false;
  bool b = false;

  if(!app || !model || !out_instance) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  #define CALL(func) \
    do { \
      if(SL_NO_ERROR != (sl_err = func)) { \
        app_err = sl_to_app_error(sl_err); \
        goto error; \
      } \
    } while(0)

  app_err = app_create_model_instance(app, &instance);
  if(app_err != APP_NO_ERROR)
    goto error;

  if(model->model_list) {
    struct rdr_model** model_lstbuf = NULL;

    SL(vector_buffer
       (model->model_list, &len, NULL, NULL, (void**)&model_lstbuf));
    for(i = 0; i < len; ++i) {
      rdr_err = rdr_create_model_instance
        (app->rdr.system, model_lstbuf[i], &render_instance);
      if(rdr_err != RDR_NO_ERROR) {
        app_err = rdr_to_app_error(rdr_err);
        goto error;
      }
      CALL(sl_vector_push_back
        (instance->model_instance_list, &render_instance));
      render_instance = NULL;
    }
  }
  app_err = app_model_ref_get(model);
  if(app_err != APP_NO_ERROR)
    goto error;
  is_ref_get = true;
  instance->model = model;

  SL(string_get(model->name, &cstr));
  CALL(sl_string_set(instance->name, "mdl_instance_"));
  CALL(sl_string_append(instance->name, cstr));
  CALL(sl_string_append(instance->name, "_0"));
  SL(string_length(instance->name, &len));
  --len;

  for(i = 0;
      APP(is_object_registered(app, APP_MODEL_INSTANCE, instance, &b)), b;
      ++i) {
    char buf[16] = { [15] = '\0' };
    SL(string_erase(instance->name, len, SIZE_MAX));
    snprintf(buf, 15, "%zu", i);
    CALL(sl_string_append(instance->name, buf));
  }

  app_err = app_register_object(app, APP_MODEL_INSTANCE, instance);
  if(app_err != APP_NO_ERROR)
    goto error;
  APP(invoke_callbacks(app, APP_SIGNAL_CREATE_MODEL_INSTANCE, instance));

  #undef CALL

exit:
  if(out_instance)
    *out_instance = instance;
  return app_err;
error:
  if(instance) {
    APP(model_instance_ref_put(instance));
    instance = NULL;
  }
  if(render_instance) {
    RDR(model_instance_ref_put(render_instance));
    render_instance = NULL;
  }
  if(is_ref_get)
    APP(model_ref_put(model));
  goto exit;
}

/*******************************************************************************
 *
 * Private app functions.
 *
 ******************************************************************************/
enum app_error
app_setup_model_set(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_create_set
    (sizeof(struct app_model*),
     ALIGNOF(struct app_model*),
     compare_models,
     app->allocator,
     &app->object_list[APP_MODEL]);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
exit:
  return app_err;
error:
  goto exit;
}

