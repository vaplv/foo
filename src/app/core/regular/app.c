#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_world_c.h"
#include "app/core/app.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "app/core/app_view.h"
#include "app/core/app_world.h"
#include "renderer/rdr_material.h"
#include "renderer/rdr_system.h"
#include "renderer/rdr_world.h"
#include "resources/rsrc_context.h"
#include "resources/rsrc_geometry.h"
#include "resources/rsrc_wavefront_obj.h"
#include "stdlib/sl_context.h"
#include "stdlib/sl_vector.h"
#include "sys/sys.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_error.h"
#include "window_manager/wm_window.h"
#include <assert.h>
#include <error.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* default_shader_sources[] = {
  [RDR_VERTEX_SHADER] =
    "#version 330\n"
    "uniform mat4x4 rdr_modelview;\n"
    "uniform mat4x4 rdr_projection;\n"
    "//uniform mat4x4 rdr_modelview_invtrans;\n"
    "in vec3 rdr_position;\n"
    "in vec3 rdr_normal;\n"
    "out vec3 r_normal;\n"
    "out vec3 r_posvs;\n"
    "void main()\n"
    "{\n"
    " //r_normal = (rdr_modelview_invtrans * vec4(rdr_normal, 1.f)).xyz;\n"
    " vec4 posvs = (rdr_modelview * vec4(rdr_position, 1.f));\n"
    " r_posvs = posvs.xyz;\n"
    " gl_Position = rdr_projection * posvs;\n"
    "}\n",
  [RDR_FRAGMENT_SHADER] =
    "#version 330\n"
    "out vec4 color;\n"
    "in vec3 r_normal;\n"
    "in vec3 r_posvs;\n"
    "void main()\n"
    "{\n"
    " vec3 n = normalize(r_normal);\n"
    " vec3 e = normalize(r_posvs);\n"
    " //color = vec4(max(dot(n, e), 0.f));\n"
    " color = vec4(1.f);\n"
    "}\n",
  [RDR_GEOMETRY_SHADER] = NULL
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static enum app_error
shutdown_window_manager(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;
  assert(app != NULL);

  if(app->window) {
    wm_err = wm_free_window(app->wm, app->window);
    if(wm_err != WM_NO_ERROR) {
      app_err = wm_to_app_error(wm_err);
      goto error;
    }
    app->window = NULL;
  }
  if(app->wm) {
    wm_err = wm_free_device(app->wm);
    if(wm_err != WM_NO_ERROR) {
      app_err = wm_to_app_error(wm_err);
      goto error;
    }
    app->wm = NULL;
  }

exit:
  return app_err;

error:
  goto exit;
}

static enum app_error
shutdown_renderer(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  assert(app != NULL);

  if(app->default_render_material) {
    rdr_err = rdr_free_material(app->rdr, app->default_render_material);
    if(rdr_err != RDR_NO_ERROR) {
      app_err = rdr_to_app_error(rdr_err);
      goto error;
    }
    app->default_render_material = NULL;
  }

  if(app->rdr) {
    rdr_err = rdr_free_system(app->rdr);
    if(rdr_err != RDR_NO_ERROR) {
      app_err = rdr_to_app_error(rdr_err);
      goto error;
    }
    app->rdr = NULL;
  }

exit:
  return app_err;

error:
  goto exit;
}

static enum app_error
shutdown_resources(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  assert(app != NULL);

  if(app->wavefront_obj) {
    rsrc_err = rsrc_free_wavefront_obj(app->rsrc, app->wavefront_obj);
    if(rsrc_err != RSRC_NO_ERROR) {
      app_err = rsrc_to_app_error(app_err);
      goto error;
    }
    app->wavefront_obj = NULL;
  }
  if(app->rsrc) {
    rsrc_err = rsrc_free_context(app->rsrc);
    if(rsrc_err != RSRC_NO_ERROR) {
      app_err = rsrc_to_app_error(app_err);
      goto error;
    }
    app->rsrc = NULL;
  }

exit:
  return app_err;

error:
  goto exit;
}

static enum app_error
shutdown_stdlib(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  assert(app != NULL);

  if(app->sl) {
    sl_err = sl_free_context(app->sl);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    app->sl = NULL;
  }

exit:
  return app_err;

error:
  goto exit;
}

static enum app_error
shutdown_common(struct app* app)
{
  void* buffer = NULL;
  size_t len = 0;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  assert(app != NULL);

  if(app->model_instance_list) {
    sl_err = sl_vector_buffer
      (app->sl, app->model_instance_list, &len, NULL, NULL, &buffer);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    for(i = 0; i < len; ++i) {
      app_err = app_free_model_instance
        (app, ((struct app_model_instance**)buffer)[i]);
      if(app_err != APP_NO_ERROR) {
        app_err = sl_to_app_error(sl_err);
        goto error;
      }
    }
    sl_err = sl_free_vector(app->sl, app->model_instance_list);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    app->model_instance_list = NULL;
  }
  if(app->model_list) {
    sl_err = sl_vector_buffer
      (app->sl, app->model_list, &len, NULL, NULL, &buffer);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    for(i = 0; i < len; ++i) {
      app_err = app_free_model(app, ((struct app_model**)buffer)[i]);
      if(app_err != APP_NO_ERROR) {
        app_err = sl_to_app_error(sl_err);
        goto error;
      }
    }
    sl_err = sl_free_vector(app->sl, app->model_list);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    app->model_list = NULL;
  }
  if(app->world) {
    app_err = app_free_world(app, app->world);
    if(app_err != APP_NO_ERROR)
      goto error;
    app->world = NULL;
  }
  if(app->view) {
    app_err = app_free_view(app, app->view);
    if(app_err != APP_NO_ERROR)
      goto error;
    app->view = NULL;
  }

exit:
  return app_err;

error:
  goto exit;
}

static enum app_error
shutdown(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;

  if(app) {
    app_err = shutdown_common(app);
    if(app_err != APP_NO_ERROR)
      goto error;
    app_err = shutdown_resources(app);
    if(app_err != APP_NO_ERROR)
      goto error;
    app_err = shutdown_stdlib(app);
    if(app_err != APP_NO_ERROR)
      goto error;
    app_err = shutdown_renderer(app);
    if(app_err != APP_NO_ERROR)
      goto error;
    app_err = shutdown_window_manager(app);
    if(app_err != APP_NO_ERROR)
      goto error;
  }

exit:
  return app_err;

error:
  goto exit;
}

static enum app_error
init_window_manager(struct app* app)
{
  const struct wm_window_desc win_desc = {
    .width = 800,
    .height = 600,
    .fullscreen = false
  };
  enum app_error app_err = APP_NO_ERROR;
  enum app_error tmp_err = APP_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;
  assert(app != NULL);

  wm_err = wm_create_device(&app->wm);
  if(wm_err != WM_NO_ERROR) {
    app_err = wm_to_app_error(wm_err);
    goto error;
  }
  wm_err = wm_create_window(app->wm, &win_desc, &app->window);
  if(wm_err != WM_NO_ERROR) {
    app_err = wm_to_app_error(wm_err);
    goto error;
  }

exit:
  return app_err;

error:
  tmp_err = shutdown_window_manager(app);
  assert(tmp_err == APP_NO_ERROR);
  goto exit;
}

static enum app_error
init_renderer(struct app* app, const char* driver)
{
  enum app_error app_err = APP_NO_ERROR;
  enum app_error tmp_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  assert(app != NULL);

  rdr_err = rdr_create_system(driver, &app->rdr);
  if(rdr_err != RDR_NO_ERROR) {
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }

  rdr_err = rdr_create_material(app->rdr, &app->default_render_material);
  if(rdr_err != RDR_NO_ERROR) {
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }

  rdr_err = rdr_material_program
    (app->rdr, app->default_render_material, default_shader_sources);
  if(rdr_err != RDR_NO_ERROR) {
    const char* log = NULL;
    RDR(get_material_log(app->rdr, app->default_render_material, &log));
    if(log  != NULL)
      fprintf(stderr, "Default render material error: \n%s\n", log);

    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }

exit:
  return app_err;

error:
  tmp_err = shutdown_renderer(app);
  assert(tmp_err == APP_NO_ERROR);
  goto exit;
}

static enum app_error
init_resources(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum app_error tmp_err = APP_NO_ERROR;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  assert(app != NULL);

  rsrc_err = rsrc_create_context(&app->rsrc);
  if(rsrc_err != RSRC_NO_ERROR) {
    app_err = rsrc_to_app_error(app_err);
    goto error;
  }
  rsrc_err = rsrc_create_wavefront_obj(app->rsrc, &app->wavefront_obj);
  if(rsrc_err != RSRC_NO_ERROR) {
    app_err = rsrc_to_app_error(app_err);
    goto error;
  }

exit:
  return app_err;

error:
  tmp_err = shutdown_resources(app);
  assert(tmp_err == APP_NO_ERROR);
  goto exit;
}

static enum app_error
init_stdlib(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum app_error tmp_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  assert(app != NULL);

  sl_err = sl_create_context(&app->sl);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }

exit:
  return app_err;

error:
  tmp_err = shutdown_stdlib(app);
  assert(tmp_err == APP_NO_ERROR);
  goto exit;
}

static enum app_error
init_common(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum app_error tmp_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  assert(app != NULL);

  sl_err = sl_create_vector
    (app->sl,
     sizeof(struct app_model*),
     ALIGNOF(struct app_model*),
     &app->model_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_create_vector
    (app->sl,
     sizeof(struct app_model_instance*),
     ALIGNOF(struct app_model_instance*),
     &app->model_instance_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  app_err = app_create_world(app, &app->world);
  if(app_err != APP_NO_ERROR)
    goto error;

  app_err = app_create_view(app, &app->view);
  if(app_err != APP_NO_ERROR)
    goto error;

exit:
  return app_err;

error:
  tmp_err = shutdown_common(app);
  assert(tmp_err == APP_NO_ERROR);
  goto exit;
}

static enum app_error
init(struct app* app, const char* graphic_driver)
{
  enum app_error app_err = APP_NO_ERROR;
  enum app_error tmp_err = APP_NO_ERROR;
  assert(app != NULL);

  app_err = init_stdlib(app);
  if(app_err != APP_NO_ERROR) {
    fprintf(stderr, "Error in initializing the standard library\n");
    goto error;
  }
  app_err = init_window_manager(app);
  if(app_err !=  APP_NO_ERROR) {
    fprintf(stderr, "Error in intializing the window manager\n");
    goto error;
  }
  app_err = init_renderer(app, graphic_driver);
  if(app_err != APP_NO_ERROR) {
    fprintf(stderr, "Error in initializing the renderer\n");
    goto error;
  }
  app_err = init_resources(app);
  if(app_err != APP_NO_ERROR) {
    fprintf(stderr, "Error in initializing the resource module\n");
    goto error;
  }
  app_err = init_common(app);
  if(app_err != APP_NO_ERROR)
    goto error;

exit:
  return app_err;

error:
  tmp_err = shutdown(app);
  assert(tmp_err == APP_NO_ERROR);
  goto exit;
}

/*******************************************************************************
 *
 * Main entry
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_init(struct app_args* args, struct app** out_app)
{
  struct app* app = NULL;
  struct app_model* mdl = NULL;
  struct app_model_instance* mdl_instance = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!args || !out_app) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  memset(&app, 0, sizeof(app));

  if(args->render_driver == NULL) {
    app_err = APP_RENDER_DRIVER_ERROR;
    goto error;
  }
  app = calloc(1, sizeof(struct app));
  if(app == NULL) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  app_err = init(app, args->render_driver);
  if(app_err != APP_NO_ERROR)
    goto error;

  if(args->model) {
    /* Create the model and add an instance of it to the world. */
    app_err = app_create_model(app, args->model, &mdl);
    if(app_err != APP_NO_ERROR)
      goto error;
    app_err = app_instantiate_model(app, mdl, &mdl_instance);
    if(app_err != APP_NO_ERROR)
      goto error;
    app_err = app_world_add_model_instances(app, app->world, 1, &mdl_instance);
    if(app_err != APP_NO_ERROR)
      goto error;

    /* Save the created model as well as its instance. */
    sl_err = sl_vector_push_back(app->sl, app->model_list, &mdl);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    sl_err = sl_vector_push_back
      (app->sl, app->model_instance_list, &mdl_instance);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
  }

exit:
  if(out_app)
    *out_app = app;
  return app_err;

error:
  if(app) {
    app_err = shutdown(app);
    assert(app_err == APP_NO_ERROR);
    free(app);
    app = NULL;
  }
  goto exit;
}

EXPORT_SYM enum app_error
app_shutdown(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;

  if(!app) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  app_err = shutdown(app);
  if(app_err != APP_NO_ERROR)
    goto error;

  free(app);

exit:
  return app_err;

error:
  goto exit;
}

EXPORT_SYM enum app_error
app_run(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;

  if(!app) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  app_err = app_draw_world(app, app->world, app->view);
  if(app_err != APP_NO_ERROR)
    goto error;

  wm_err = wm_swap(app->wm, app->window);
  if(wm_err != WM_NO_ERROR) {
    app_err = wm_to_app_error(wm_err);
    goto error;
  }
                  
exit:
  return app_err;

error:
  goto exit;
}

EXPORT_SYM enum app_error
app_get_window_manager_device(struct app* app, struct wm_device** out_wm)
{
  if(!app || !out_wm)
    return APP_INVALID_ARGUMENT;
  *out_wm = app->wm;
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_get_main_view(struct app* app, struct app_view** out_view)
{
  if(!app || !out_view)
    return APP_INVALID_ARGUMENT;
  *out_view = app->view;
  return APP_NO_ERROR;
}

/*******************************************************************************
 *
 * Private functions.
 *
 ******************************************************************************/
enum rdr_attrib_usage
rsrc_to_rdr_attrib_usage(enum rsrc_attrib_usage usage)
{
  enum rdr_attrib_usage attr_usage = RDR_ATTRIB_UNKNOWN;
  switch(usage) {
    case RSRC_ATTRIB_POSITION:
      attr_usage = RDR_ATTRIB_POSITION;
      break;
    case RSRC_ATTRIB_NORMAL:
      attr_usage = RDR_ATTRIB_NORMAL;
      break;
    case RSRC_ATTRIB_TEXCOORD:
      attr_usage = RDR_ATTRIB_TEXCOORD;
      break;
    default:
      attr_usage = RDR_ATTRIB_UNKNOWN;
      break;
  }
  return attr_usage;
}

enum rdr_type
rsrc_to_rdr_type(enum rsrc_type type)
{
  enum rdr_type rdr_type = RDR_UNKNOWN_TYPE;
  switch(type) {
    case RSRC_FLOAT:
      rdr_type = RDR_FLOAT;
      break;
    case RSRC_FLOAT2:
      rdr_type = RDR_FLOAT2;
      break;
    case RSRC_FLOAT3:
      rdr_type = RDR_FLOAT3;
      break;
    case RSRC_FLOAT4:
      rdr_type = RDR_FLOAT4;
      break;
    default:
      rdr_type = RDR_UNKNOWN_TYPE;
      break;
  }
  return rdr_type;
}

