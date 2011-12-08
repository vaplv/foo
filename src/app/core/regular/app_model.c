#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_model_c.h"
#include "app/core/regular/app_model_instance_c.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "renderer/rdr_material.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_model.h"
#include "renderer/rdr_model_instance.h"
#include "resources/rsrc_geometry.h"
#include "resources/rsrc_wavefront_obj.h"
#include "stdlib/sl_string.h"
#include "stdlib/sl_sorted_vector.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct app_model {
  struct ref ref;
  struct app* app;
  struct sl_string* name;
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
      RDR(free_mesh(model->app->rdr, mesh_lstbuf[i]));
    SL(clear_vector(model->mesh_list));
  }
  if(model->material_list) {
    SL(vector_buffer
       (model->material_list, &len, NULL, NULL, (void**)&mtr_lstbuf));
    for(i = 0; i < len; ++i)
      RDR(free_material(model->app->rdr, mtr_lstbuf[i]));
    SL(clear_vector(model->material_list));
  }
  if(model->model_list) {
    SL(vector_buffer
       (model->model_list, &len, NULL, NULL, (void**)&mdl_lstbuf));
    for(i = 0; i < len; ++i)
      RDR(free_model(model->app->rdr, mdl_lstbuf[i]));
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

  RSRC(get_primitive_set_count
    (model->app->rsrc, model->geometry, &nb_prim_set));
  for(i = 0; i < nb_prim_set; ++i) {
    struct rsrc_primitive_set prim_set;
    size_t j = 0;
    RSRC(get_primitive_set(model->app->rsrc, model->geometry, i, &prim_set));

    /* Only triangular geometry are handled. */
    if(prim_set.primitive_type != RSRC_TRIANGLE)
      continue;

    /* Render mesh setup. */
    rdr_err = rdr_create_mesh(model->app->rdr, &mesh);
    if(rdr_err != RDR_NO_ERROR) {
      app_err = rdr_to_app_error(rdr_err);
      goto error;
    }
    rdr_err = rdr_mesh_indices
      (model->app->rdr, mesh, prim_set.nb_indices, prim_set.index_list);
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
      (model->app->rdr,
       mesh,
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
      (model->app->rdr, mesh, model->app->default_render_material, &rmodel);
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
    RDR(free_mesh(model->app->rdr, mesh));
  if(rmodel)
    RDR(free_model(model->app->rdr, rmodel));
  goto exit;
}

static void
release_model(struct ref* ref)
{
  struct app_model* model = CONTAINER_OF(ref, struct app_model, ref);
  bool was_registered = true;
  assert(ref != NULL);

  APP(clear_model(model));

  if(model->name)
    SL(free_string(model->name));
  if(model->geometry)
    RSRC(free_geometry(model->app->rsrc, model->geometry));
  if(model->mesh_list)
    SL(free_vector(model->mesh_list));
  if(model->material_list)
    SL(free_vector(model->material_list));
  if(model->model_list)
    SL(free_vector(model->model_list));

  APP(is_model_registered(model, &was_registered));
  if(was_registered)
    APP(unregister_model(model));

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
  mdl->app = app;
  ref_init(&mdl->ref);

  sl_err = sl_create_string(NULL, app->allocator, &mdl->name);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  rsrc_err = rsrc_create_geometry(app->rsrc, &mdl->geometry);
  if(rsrc_err != RSRC_NO_ERROR) {
    app_err = rsrc_to_app_error(rsrc_err);
    goto error;
  }
  sl_err = sl_create_vector
    (sizeof(struct rdr_mesh*),
     ALIGNOF(struct rdr_mesh*),
     app->allocator,
     &mdl->mesh_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_create_vector
    (sizeof(struct rdr_material*),
     ALIGNOF(struct rdr_material*),
     app->allocator,
     &mdl->material_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_create_vector
    (sizeof(struct rdr_model*),
     ALIGNOF(struct rdr_model*),
     app->allocator,
     &mdl->model_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  if(path) {
    app_err = app_load_model(path, mdl);
    if(app_err != APP_NO_ERROR)
      goto error;
  }
  app_err = app_register_model(mdl);
  if(app_err != APP_NO_ERROR)
    goto error;

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

  if(model->geometry)
    RSRC(clear_geometry(model->app->rsrc, model->geometry));

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

  if(!path || !model) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  assert(model->app && model->app->wavefront_obj != NULL);

  rsrc_err = rsrc_load_wavefront_obj
    (model->app->rsrc, model->app->wavefront_obj, path);
  if(rsrc_err != RSRC_NO_ERROR) {
    APP_LOG_ERR(model->app, "Error loading the geometry `%s'\n", path);
    app_err = rsrc_to_app_error(rsrc_err);
    goto error;
  }
  rsrc_err = rsrc_geometry_from_wavefront_obj
    (model->app->rsrc, model->geometry, model->app->wavefront_obj);
  if(rsrc_err != RSRC_NO_ERROR) {
    app_err = rsrc_to_app_error(rsrc_err);
    goto error;
  }
  app_err = setup_model(model);
  if(app_err != APP_NO_ERROR) {
    goto error;
  }
  sl_err = sl_string_set(model->name, path);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }

exit:
  return app_err;

error:
  if(model) {
    UNUSED const enum app_error err = app_clear_model(model);
    assert(err == APP_NO_ERROR);
  }
  goto exit;
}

EXPORT_SYM enum app_error
app_model_name
  (struct app_model* model,
   const char* name)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!model || !name) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_string_set(model->name, name);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }

exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_get_model_name
  (const struct app_model* model,
   const char** name)
{
  enum app_error app_err = APP_NO_ERROR;
  bool name_is_empty = false;

  if(!model || !name) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  SL(is_string_empty(model->name, &name_is_empty));
  if(name_is_empty) {
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
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app || !model || !out_instance) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  app_err = app_create_model_instance(app, &instance);
  if(app_err != APP_NO_ERROR)
    goto error;

  if(model->model_list) {
    struct rdr_model** model_lstbuf = NULL;
    size_t len = 0;
    size_t i = 0;

    SL(vector_buffer
       (model->model_list, &len, NULL, NULL, (void**)&model_lstbuf));
    for(i = 0; i < len; ++i) {
      rdr_err = rdr_create_model_instance
        (app->rdr, model_lstbuf[i], &render_instance);
      if(rdr_err != RDR_NO_ERROR) {
        app_err = rdr_to_app_error(rdr_err);
        goto error;
      }
      sl_err = sl_vector_push_back
        (instance->model_instance_list, &render_instance);
      if(sl_err != SL_NO_ERROR) {
        app_err = sl_to_app_error(sl_err);
        goto error;
      }
      render_instance = NULL;
    }
  }
  app_err = app_register_model_instance(instance);
  if(app_err != APP_NO_ERROR)
    goto error;

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
    RDR(free_model_instance(app->rdr, render_instance));
    render_instance = NULL;
  }
  goto exit;
}

/*******************************************************************************
 *
 * Private functions.
 *
 ******************************************************************************/
enum app_error
app_register_model(struct app_model* model)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  struct model_callback* cbk_list = NULL;
  size_t len = 0;
  size_t i = 0;

  if(!model) {
    app_err = APP_NO_ERROR;
    goto error;
  }
  sl_err = sl_sorted_vector_insert(model->app->model_list, &model);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_sorted_vector_buffer
    (model->app->add_model_cbk_list, &len, NULL, NULL, (void**)&cbk_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  for(i = 0; i < len; ++i)
    cbk_list[i].func(model, cbk_list[i].data);

exit:
  return app_err;
error:
  goto exit;
}

enum app_error
app_unregister_model(struct app_model* model)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  struct model_callback* cbk_list = NULL;
  size_t len = 0;
  size_t i = 0;

  if(!model) {
    app_err = APP_NO_ERROR;
    goto error;
  }
  sl_err = sl_sorted_vector_remove(model->app->model_list, &model);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_sorted_vector_buffer
    (model->app->remove_model_cbk_list, &len, NULL, NULL, (void**)&cbk_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  for(i = 0; i < len; ++i)
    cbk_list[i].func(model, cbk_list[i].data);

exit:
  return app_err;
error:
  goto exit;
}

enum app_error
app_is_model_registered(struct app_model* model, bool* is_registered)
{
  enum app_error app_err = APP_NO_ERROR;
  size_t i = 0;
  size_t len = 0;

  if(!model || !is_registered) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(sorted_vector_find(model->app->model_list, &model, &i));
  SL(sorted_vector_buffer(model->app->model_list, &len, NULL, NULL, NULL));
  *is_registered = (i != len);

exit:
  return app_err;
error:
  goto exit;
}

