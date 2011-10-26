#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_model_instance_c.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "renderer/rdr_material.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_model.h"
#include "renderer/rdr_model_instance.h"
#include "resources/rsrc_geometry.h"
#include "resources/rsrc_wavefront_obj.h"
#include "stdlib/sl_vector.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct app_model {
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
clear_model_render_data(struct app* app, struct app_model* model)
{
  struct rdr_mesh** mesh_lstbuf = NULL;
  struct rdr_material** mtr_lstbuf = NULL;
  struct rdr_model** mdl_lstbuf = NULL;
  size_t i = 0;
  size_t len = 0;

  assert(app && model);

  if(model->mesh_list) {
    SL(vector_buffer
       (app->sl, model->mesh_list, &len, NULL, NULL, (void**)&mesh_lstbuf));
    for(i = 0; i < len; ++i)
      RDR(free_mesh(app->rdr, mesh_lstbuf[i]));
    SL(clear_vector(app->sl, model->mesh_list));
  }
  if(model->material_list) {
    SL(vector_buffer
       (app->sl, model->material_list, &len, NULL, NULL, (void**)&mtr_lstbuf));
    for(i = 0; i < len; ++i)
      RDR(free_material(app->rdr, mtr_lstbuf[i]));
    SL(clear_vector(app->sl, model->material_list));
  }
  if(model->model_list) {
    SL(vector_buffer
       (app->sl, model->model_list, &len, NULL, NULL, (void**)&mdl_lstbuf));
    for(i = 0; i < len; ++i)
      RDR(free_model(app->rdr, mdl_lstbuf[i]));
    SL(clear_vector(app->sl, model->model_list));
  }
  return APP_NO_ERROR;
}

static enum app_error
setup_model(struct app* app, struct app_model* model)
{
  struct rdr_mesh* mesh = NULL;
  struct rdr_model* rmodel = NULL;
  size_t nb_prim_set = 0;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  assert(app && model && model->geometry);

  app_err = clear_model_render_data(app, model);
  if(app_err != APP_NO_ERROR)
    goto error;

  RSRC(get_primitive_set_count(app->rsrc, model->geometry, &nb_prim_set));
  for(i = 0; i < nb_prim_set; ++i) {
    struct rsrc_primitive_set prim_set;
    size_t j = 0;
    RSRC(get_primitive_set(app->rsrc, model->geometry, i, &prim_set));

    /* Only triangular geometry are handled. */
    if(prim_set.primitive_type != RSRC_TRIANGLE)
      continue;

    /* Render mesh setup. */
    rdr_err = rdr_create_mesh(app->rdr, &mesh);
    if(rdr_err != RDR_NO_ERROR) {
      app_err = rdr_to_app_error(rdr_err);
      goto error;
    }
    rdr_err = rdr_mesh_indices
      (app->rdr, mesh, prim_set.nb_indices, prim_set.index_list);
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
      (app->rdr,
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
      (app->rdr, mesh, app->default_render_material, &rmodel);
    if(rdr_err != RDR_NO_ERROR) {
      app_err = rdr_to_app_error(rdr_err);
      goto error;
    }

    /* Register the mesh and the model. */
    sl_err = sl_vector_push_back(app->sl, model->mesh_list, &mesh);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    mesh = NULL;
    sl_err = sl_vector_push_back(app->sl, model->model_list, &rmodel);
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
    RDR(free_mesh(app->rdr, mesh));
  if(rmodel)
    RDR(free_model(app->rdr, rmodel));
  goto exit;
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

  mdl = calloc(1, sizeof(struct app_model));
  if(NULL == mdl) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  rsrc_err = rsrc_create_geometry(app->rsrc, &mdl->geometry);
  if(rsrc_err != RSRC_NO_ERROR) {
    app_err = rsrc_to_app_error(rsrc_err);
    goto error;
  }
  sl_err = sl_create_vector
    (app->sl,
     sizeof(struct rdr_mesh*),
     ALIGNOF(struct rdr_mesh*),
     &mdl->mesh_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_create_vector
    (app->sl,
     sizeof(struct rdr_material*),
     ALIGNOF(struct rdr_material*),
     &mdl->material_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_create_vector
    (app->sl,
     sizeof(struct rdr_model*),
     ALIGNOF(struct rdr_model*),
     &mdl->model_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  if(path) {
    app_err = app_load_model(app, path, mdl);
    if(app_err != APP_NO_ERROR)
      goto error;
  }

exit:
  if(model)
    *model = mdl;
  return app_err;

error:
  if(mdl) {
    UNUSED const enum app_error err = app_free_model(app, mdl);
    assert(err == APP_NO_ERROR);
    mdl = NULL;
  }
  goto exit;
}

EXPORT_SYM enum app_error
app_free_model(struct app* app, struct app_model* model)
{
  enum app_error app_err = APP_NO_ERROR;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app || !model) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  app_err = app_clear_model(app, model);
  if(app_err != APP_NO_ERROR)
    goto error;

  rsrc_err = rsrc_free_geometry(app->rsrc, model->geometry);
  if(rsrc_err != RSRC_NO_ERROR) {
    app_err = rsrc_to_app_error(rsrc_err);
    goto error;
  }
  sl_err = sl_free_vector(app->sl, model->mesh_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_free_vector(app->sl, model->material_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_free_vector(app->sl, model->model_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  free(model);

exit:
  return app_err;

error:
  goto exit;
}

EXPORT_SYM enum app_error
app_clear_model(struct app* app, struct app_model* model)
{
  enum app_error app_err = APP_NO_ERROR;

  if(!app || !model) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  if(model->geometry)
    RSRC(clear_geometry(app->rsrc, model->geometry));

  app_err = clear_model_render_data(app, model);
  if(app_err != APP_NO_ERROR)
    goto error;

exit:
  return app_err;

error:
  goto exit;
}

EXPORT_SYM enum app_error
app_load_model(struct app* app, const char* path, struct app_model* model)
{
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  enum app_error app_err = APP_NO_ERROR;

  if(!app || !path || !model) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  assert(app->wavefront_obj != NULL);

  rsrc_err = rsrc_load_wavefront_obj(app->rsrc, app->wavefront_obj, path);
  if(rsrc_err != RSRC_NO_ERROR) {
    fprintf(stderr, "Error loading the geometry `%s'\n", path);
    app_err = rsrc_to_app_error(rsrc_err);
    goto error;
  }
  rsrc_err = rsrc_geometry_from_wavefront_obj
    (app->rsrc, model->geometry, app->wavefront_obj);
  if(rsrc_err != RSRC_NO_ERROR) {
    app_err = rsrc_to_app_error(rsrc_err);
    goto error;
  }
  app_err = setup_model(app, model);
  if(app_err != APP_NO_ERROR)
    goto error;

exit:
  return app_err;

error:
  if(app && model) {
    UNUSED const enum app_error err = app_clear_model(app, model);
    assert(err == APP_NO_ERROR);
  }
  goto exit;
}

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
       (app->sl, model->model_list, &len, NULL, NULL, (void**)&model_lstbuf));
    for(i = 0; i < len; ++i) {
      rdr_err = rdr_create_model_instance
        (app->rdr, model_lstbuf[i], &render_instance);
      if(rdr_err != RDR_NO_ERROR) {
        app_err = rdr_to_app_error(rdr_err);
        goto error;
      }
      sl_err = sl_vector_push_back
        (app->sl, instance->model_instance_list, &render_instance);
      if(sl_err != SL_NO_ERROR) {
        app_err = sl_to_app_error(sl_err);
        goto error;
      }
      render_instance = NULL;
    }
  }

exit:
  if(out_instance)
    *out_instance = instance;
  return app_err;

error:
  if(instance) {
    app_err = app_free_model_instance(app, instance);
    assert(app_err == APP_NO_ERROR);
    instance = NULL;
  }
  if(render_instance) {
    RDR(free_model_instance(app->rdr, render_instance));
    render_instance = NULL;
  }
  goto exit;
}

