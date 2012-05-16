#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_model_c.h"
#include "app/core/regular/app_model_instance_c.h"
#include "app/core/regular/app_object.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "renderer/rdr.h"
#include "renderer/rdr_material.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_model.h"
#include "renderer/rdr_model_instance.h"
#include "resources/rsrc_context.h"
#include "resources/rsrc_geometry.h"
#include "resources/rsrc_wavefront_obj.h"
#include "stdlib/sl.h"
#include "stdlib/sl_flat_map.h"
#include "stdlib/sl_string.h"
#include "stdlib/sl_vector.h"
#include "sys/math.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct app_model {
  float min_bound[3]; /* min bound of the union of the rdr mesh aabb. */
  float max_bound[3]; /* max bound of the union of the rdr_mesh aabb. */
  struct app_object obj;
  struct ref ref;
  struct app* app;
  struct sl_string* resource_path;
  struct rsrc_geometry* geometry;
  struct list_node instance_list; /* list of the instances spawned from this. */
  struct sl_vector* mesh_list; /* List of rdr_mesh*. */
  struct sl_vector* material_list; /* list of rdr_material*. */
  struct sl_vector* model_list; /* list of rdr_model*. */
  bool invoke_clbk;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
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

static FINLINE void
set_default_model_bounds(struct app_model* mdl)
{
  assert(mdl);
  mdl->min_bound[0] = mdl->min_bound[1] = mdl->min_bound[2] = FLT_MAX;
  mdl->max_bound[0] = mdl->max_bound[1] = mdl->max_bound[2] = -FLT_MAX;
}

static enum app_error
setup_model(struct app_model* model)
{
  float min_bound[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
  float max_bound[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
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

  set_default_model_bounds(model);
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
    /* Update the model AABB. */
    RDR(get_mesh_aabb(mesh, min_bound, max_bound));
    model->min_bound[0] = MIN(model->min_bound[0], min_bound[0]);
    model->min_bound[1] = MIN(model->min_bound[1], min_bound[1]);
    model->min_bound[2] = MIN(model->min_bound[2], min_bound[2]);
    model->max_bound[0] = MAX(model->max_bound[0], max_bound[0]);
    model->max_bound[1] = MAX(model->max_bound[1], max_bound[1]);
    model->max_bound[2] = MAX(model->max_bound[2], max_bound[2]);
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
  struct app* app = NULL;
  struct app_model* model = NULL;
  assert(ref != NULL);

  model = CONTAINER_OF(ref, struct app_model, ref);
  assert(is_list_empty(&model->instance_list));

  if(model->invoke_clbk)
    APP(invoke_callbacks(model->app, APP_SIGNAL_DESTROY_MODEL, model));

  APP(clear_model(model));

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

  APP(release_object(model->app, &model->obj));

  app = model->app;
  MEM_FREE(model->app->allocator, model);
  APP(ref_put(app));
}

static void
release_model_object(struct app_object* obj)
{
  assert(obj);
  APP(model_ref_put(app_object_to_model(obj)));
}

/*******************************************************************************
 *
 *  Implementation of the app_model functions.
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_create_model
  (struct app* app,
   const char* path,
   const char* name,
   struct app_model** model)
{
  struct app_model* mdl = NULL;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  enum app_error app_err = APP_NO_ERROR;

  if(!app || !model) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  mdl = MEM_CALLOC(app->allocator, 1, sizeof(struct app_model));
  if(NULL == mdl) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  ref_init(&mdl->ref);
  list_init(&mdl->instance_list);
  APP(ref_get(app));
  mdl->app = app;
  set_default_model_bounds(mdl);

  app_err = app_init_object
    (app, &mdl->obj, APP_MODEL, release_model_object, name);
  if(app_err != APP_NO_ERROR)
    goto error;

  #define CALL(func) \
    do { \
      if(SL_NO_ERROR != (sl_err = func)) { \
        app_err = sl_to_app_error(sl_err); \
        goto error; \
      } \
    } while(0)

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
  APP(invoke_callbacks(app, APP_SIGNAL_CREATE_MODEL, mdl));
  mdl->invoke_clbk = true;

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

  if(model->resource_path) {
    const enum sl_error sl_err = sl_clear_string(model->resource_path);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
  }
  if(model->geometry) {
    RSRC(clear_geometry(model->geometry));
  }
  app_err = clear_model_render_data(model);
  if(app_err != APP_NO_ERROR)
    goto error;
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_remove_model(struct app_model* model)
{
  struct list_node* node = NULL;
  struct list_node* tmp = NULL;
  bool is_registered = false;

  if(!model)
    return APP_INVALID_ARGUMENT;

  APP(is_object_registered(model->app, &model->obj, &is_registered));
  if(is_registered == false)
    return APP_INVALID_ARGUMENT;

  /* Remove all of its associated instance. */
  LIST_FOR_EACH_SAFE(node, tmp, &model->instance_list) {
    struct app_model_instance* instance = CONTAINER_OF
      (node, struct app_model_instance, model_node);
    APP(remove_model_instance(instance));
  }

  if(ref_put(&model->ref, release_model) == 0)
    APP(unregister_object(model->app, &model->obj));
  return APP_NO_ERROR;
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
  const char* str_err = NULL;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool may_have_errors = false;

  if(!path || !model) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  assert(model->app && model->app->rsrc.wavefront_obj != NULL);

  RSRC(flush_error(model->app->rsrc.context));
  may_have_errors = true;

  rsrc_err = rsrc_load_wavefront_obj
    (model->app->rsrc.wavefront_obj, path);
  if(rsrc_err != RSRC_NO_ERROR) {
    APP_PRINT_ERR
      (model->app->logger, "error loading geometry resource `%s'\n", path);
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

exit:
  return app_err;
error:
  if(model)
    APP(clear_model(model));
  if(may_have_errors) {
    RSRC(get_error_string(model->app->rsrc.context, &str_err));
    if(str_err) {
      APP_PRINT_ERR(model->app->logger, "%s", str_err);
    }
  }
  goto exit;
}

EXPORT_SYM enum app_error
app_model_path
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
  if(!model)
    return APP_INVALID_ARGUMENT;
  return app_set_object_name(model->app, &model->obj, name);
}

EXPORT_SYM enum app_error
app_model_name(const struct app_model* model, const char** name)
{
  if(!model)
    return APP_INVALID_ARGUMENT;
  return app_object_name(model->app, &model->obj, name);
}

EXPORT_SYM enum app_error
app_is_model_instantiated(const struct app_model* model, bool* b)
{
  if(!model || !b)
    return APP_INVALID_ARGUMENT;
  *b = (is_list_empty(&model->instance_list) == 0);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_get_model(struct app* app, const char* mdl_name, struct app_model** mdl)
{
  struct app_object* obj = NULL;
  enum app_error app_err = APP_NO_ERROR;

  if(!mdl) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  app_err = app_get_object(app, APP_MODEL, mdl_name, &obj);
  if(app_err != APP_NO_ERROR)
    goto error;

  if(obj) {
    *mdl = app_object_to_model(obj);
  } else {
    *mdl = NULL;
  }
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_get_model_list_begin
  (struct app* app,
   struct app_model_it* it,
   bool* is_end_reached)
{
  struct app_object** obj_list = NULL;
  size_t len = 0;

  if(UNLIKELY(!app || !it || !is_end_reached))
    return APP_INVALID_ARGUMENT;

  APP(get_object_list(app, APP_MODEL, &len, &obj_list));
  if(len == 0) {
    *is_end_reached = true;
  } else {
    it->model = app_object_to_model(obj_list[0]);
    it->id = 0;
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_model_it_next(struct app_model_it* it, bool* is_end_reached)
{
  struct app_object** obj_list = NULL;
  size_t len = 0;

  if(UNLIKELY(!it || !is_end_reached || !it->model))
    return APP_INVALID_ARGUMENT;
  APP(get_object_list(it->model->app, APP_MODEL, &len, &obj_list));
  ++it->id;
  if(len <=it->id) {
    *is_end_reached = true;
  } else {
    it->model = app_object_to_model(obj_list[it->id]);
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_get_model_list_length(struct app* app, size_t* len)
{
  if(UNLIKELY(!app || !len))
    return APP_INVALID_ARGUMENT;
  APP(get_object_list(app, APP_MODEL, len, NULL));
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_model_name_completion
  (struct app* app,
   const char* mdl_name,
   size_t mdl_name_len,
   size_t* completion_list_len,
   const char** completion_list[])
{
  return app_object_name_completion
    (app,
     APP_MODEL,
     mdl_name,
     mdl_name_len,
     completion_list_len,
     completion_list);
}

/*******************************************************************************
 *
 * Private model function
 *
 ******************************************************************************/
struct app_model*
app_object_to_model(struct app_object* obj)
{
  assert(obj && obj->type == APP_MODEL);
  return CONTAINER_OF(obj, struct app_model, obj);
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
   const char* name,
   struct app_model_instance** out_instance)
{
  struct app_model_instance* instance = NULL;
  struct rdr_model_instance* render_instance = NULL;
  size_t len = 0;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool is_ref_get = false;

  if(!app || !model) {
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

  app_err = app_create_model_instance(app, name, &instance);
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
  list_add(&model->instance_list, &instance->model_node);

  APP(invoke_callbacks(app, APP_SIGNAL_CREATE_MODEL_INSTANCE, instance));
  instance->invoke_clbk = true;

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

