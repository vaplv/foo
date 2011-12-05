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
#include "stdlib/sl_logger.h"
#include "stdlib/sl_sorted_vector.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
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
#include <strings.h>

static const char* default_shader_sources[] = {
  [RDR_VERTEX_SHADER] =
    "#version 330\n"
    "uniform mat4x4 rdr_modelview;\n"
    "uniform mat4x4 rdr_projection;\n"
    "uniform mat4x4 rdr_modelview_invtrans;\n"
    "in vec3 rdr_position;\n"
    "in vec3 rdr_normal;\n"
    "smooth out vec3 r_normal;\n"
    "smooth out vec3 r_posvs;\n"
    "void main()\n"
    "{\n"
    " vec4 normal = (rdr_modelview_invtrans * vec4(rdr_normal, 1.f));\n"
    " r_normal = normal.xyz;\n"
    " vec4 posvs = (rdr_modelview * vec4(rdr_position, 1.f));\n"
    " r_posvs = posvs.xyz;\n"
    " gl_Position = rdr_projection * posvs;\n"
    "}\n",
  [RDR_FRAGMENT_SHADER] =
    "#version 330\n"
    "out vec4 color;\n"
    "smooth in vec3 r_posvs;\n"
    "smooth in vec3 r_normal;\n"
    "void main()\n"
    "{\n"
    " vec3 n = normalize(r_normal);\n"
    " vec3 e = normalize(r_posvs);\n"
    " color = vec4(dot(-e, n));\n"
    "}\n",
  [RDR_GEOMETRY_SHADER] = NULL
};

/*******************************************************************************
 *
 * Callbacks.
 *
 ******************************************************************************/
enum callback_action {
  CALLBACK_CONNECT,
  CALLBACK_DISCONNECT
};

struct model_callback {
  void (*func)(struct app*, struct app_model*, void*);
  void* data;
};

static int
compare_callbacks(const void* a, const void* b)
{
  struct model_callback* cbk0 = (struct model_callback*)a;
  struct model_callback* cbk1 = (struct model_callback*)b;
  const uintptr_t p0[2] = {(uintptr_t)cbk0->func, (uintptr_t)cbk0->data};
  const uintptr_t p1[2] = {(uintptr_t)cbk1->func, (uintptr_t)cbk1->data};
  const int inf = (p0[0] < p1[0]) | ((p0[0] == p1[0]) & (p0[1] < p1[1]));
  const int sup = (p0[0] > p1[0]) | ((p0[0] == p1[0]) & (p0[1] > p1[1]));
  return -(inf) | (sup);
}

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
std_log_func(const char* msg, void* data UNUSED)
{
  /* -1 <=> NULL terminated character. */
  if(strncasecmp(msg, APP_ERR_PREFIX, sizeof(APP_ERR_PREFIX)-1) == 0) {
    fprintf(stdout, "\033[31m%s\033[0m", msg);
  } else {
    fprintf(stdout, "%s\n", msg);
  }
}

static int
compare_pointers(const void* ptr0, const void* ptr1)
{
  const uintptr_t a = (uintptr_t)(*(void**)ptr0);
  const uintptr_t b = (uintptr_t)(*(void**)ptr1);
  return -(a < b) | (a > b);
}

static enum app_error
manage_model_callback_list
  (struct app* app,
   enum app_model_event event,
   void (*func)(struct app*, struct app_model*, void*),
   void* data,
   enum callback_action action)
{
  struct sl_sorted_vector* svec = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool is_updated = false;

  if(!app || !func) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  switch(event) {
    case APP_MODEL_EVENT_ADD:
      svec = app->add_model_cbk_list;
      break;
    case APP_MODEL_EVENT_REMOVE:
      svec = app->remove_model_cbk_list;
      break;
    default:
      assert(false);
      break;
  }
  switch(action) {
    case CALLBACK_CONNECT:
      sl_err = sl_sorted_vector_insert
        (svec, (struct model_callback[]){{func, data}});
      break;
    case CALLBACK_DISCONNECT:
      sl_err = sl_sorted_vector_remove
        (svec, (struct model_callback[]){{func, data}});
      break;
    default:
      assert(false);
      break;
  }
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  is_updated = true;

exit:
  return app_err;

error:
  if(is_updated) {
    switch(action) {
      case CALLBACK_CONNECT:
        SL(sorted_vector_remove(svec, (struct model_callback[]){{func, data}}));
        break;
      case CALLBACK_DISCONNECT:
        SL(sorted_vector_insert(svec, (struct model_callback[]){{func, data}}));
        break;
      default:
        assert(false);
        break;
    }
  }
  goto exit;
}

static enum app_error
shutdown_window_manager(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;
  assert(app != NULL);

  if(app->wm) {
    if(app->window) {
      wm_err = wm_free_window(app->wm, app->window);
      if(wm_err != WM_NO_ERROR) {
        app_err = wm_to_app_error(wm_err);
        goto error;
      }
      app->window = NULL;
    }
    wm_err = wm_free_device(app->wm);
    if(wm_err != WM_NO_ERROR) {
      app_err = wm_to_app_error(wm_err);
      goto error;
    }
    app->wm = NULL;

    if(MEM_ALLOCATED_SIZE(&app->wm_allocator)) {
      char dump[BUFSIZ];
      MEM_DUMP(&app->wm_allocator, dump, BUFSIZ);
      APP_LOG_MSG(app, "Window manager leaks summary:\n%s\n", dump);
    }
    mem_shutdown_proxy_allocator(&app->wm_allocator);
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

  if(app->rdr) {
    if(app->default_render_material) {
      rdr_err = rdr_free_material(app->rdr, app->default_render_material);
      if(rdr_err != RDR_NO_ERROR) {
        app_err = rdr_to_app_error(rdr_err);
        goto error;
      }
      app->default_render_material = NULL;
    }
    rdr_err = rdr_free_system(app->rdr);
    if(rdr_err != RDR_NO_ERROR) {
      app_err = rdr_to_app_error(rdr_err);
      goto error;
    }
    app->rdr = NULL;

    if(MEM_ALLOCATED_SIZE(&app->rdr_allocator)) {
      char dump[BUFSIZ];
      MEM_DUMP(&app->rdr_allocator, dump, BUFSIZ);
      APP_LOG_MSG(app, "Renderer leaks summary:\n%s\n", dump);
    }
    mem_shutdown_proxy_allocator(&app->rdr_allocator);
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

  if(app->rsrc) {
    if(app->wavefront_obj) {
      rsrc_err = rsrc_free_wavefront_obj(app->rsrc, app->wavefront_obj);
      if(rsrc_err != RSRC_NO_ERROR) {
        app_err = rsrc_to_app_error(app_err);
        goto error;
      }
      app->wavefront_obj = NULL;
    }
    rsrc_err = rsrc_free_context(app->rsrc);
    if(rsrc_err != RSRC_NO_ERROR) {
      app_err = rsrc_to_app_error(app_err);
      goto error;
    }
    app->rsrc = NULL;

    if(MEM_ALLOCATED_SIZE(&app->rsrc_allocator)) {
      char dump[BUFSIZ];
      MEM_DUMP(&app->rsrc_allocator, dump, BUFSIZ);
      APP_LOG_MSG(app, "Resource leaks summary:\n%s\n", dump);
    }
    mem_shutdown_proxy_allocator(&app->rsrc_allocator);
  }

exit:
  return app_err;

error:
  goto exit;
}

static enum app_error
shutdown_common(struct app* app)
{
  size_t len = 0;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;
  assert(app != NULL);

  /* Note that when the model/model_instance is freed, it is unregistered from
   * the application, i.e. it is removed from the app->model_[instance_]list. */
  if(app->model_instance_list) {
    SL(sorted_vector_buffer(app->model_instance_list, &len, NULL, NULL, NULL));
    for(i = len; i > 0; ) {
      struct app_model_instance** instance = NULL;
      SL(sorted_vector_at(app->model_instance_list, --i, (void**)&instance));
      APP(free_model_instance(app, *instance));
    }
    SL(free_sorted_vector(app->model_instance_list));
    app->model_instance_list = NULL;
  }
  if(app->model_list) {
    SL(sorted_vector_buffer(app->model_list, &len, NULL, NULL, NULL));
    for(i = len; i > 0; ) {
      struct app_model** model = NULL;
      SL(sorted_vector_at(app->model_list, --i, (void**)&model));
      APP(free_model(app, *model));
    }
    SL(free_sorted_vector(app->model_list));
    app->model_list = NULL;
  }
  if(app->add_model_cbk_list) {
    SL(free_sorted_vector(app->add_model_cbk_list));
    app->add_model_cbk_list = NULL;
  }
  if(app->remove_model_cbk_list) {
    SL(free_sorted_vector(app->remove_model_cbk_list));
    app->remove_model_cbk_list = NULL;
  }
  if(app->world) {
    APP(free_world(app, app->world));
  }
  if(app->view) {
    APP(free_view(app, app->view));
  }

  return app_err;
}

static enum app_error
shutdown_sys(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  assert(app);

  if(app->logger) {
    sl_err = sl_free_logger(app->logger);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
    app->logger = NULL;
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
    app_err = shutdown_renderer(app);
    if(app_err != APP_NO_ERROR)
      goto error;
    app_err = shutdown_window_manager(app);
    if(app_err != APP_NO_ERROR)
      goto error;
    app_err = shutdown_sys(app);
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

  mem_init_proxy_allocator
    ("window manager:", &app->wm_allocator, &mem_default_allocator);
  wm_err = wm_create_device(&app->wm_allocator, &app->wm);
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

  mem_init_proxy_allocator
    ("renderer", &app->rdr_allocator, &mem_default_allocator);
  rdr_err = rdr_create_system(driver, &app->rdr_allocator, &app->rdr);
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
      APP_LOG_ERR(app, "Default render material error: \n%s", log);

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

  mem_init_proxy_allocator
    ("resources", &app->rsrc_allocator, &mem_default_allocator);
  rsrc_err = rsrc_create_context(&app->rsrc_allocator, &app->rsrc);
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
init_common(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum app_error tmp_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  assert(app != NULL);

  sl_err = sl_create_sorted_vector
    (sizeof(struct app_model*),
     ALIGNOF(struct app_model*),
     compare_pointers,
     app->allocator,
     &app->model_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_create_sorted_vector
    (sizeof(struct app_model_instance*),
     ALIGNOF(struct app_model_instance*),
     compare_pointers,
     app->allocator,
     &app->model_instance_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_create_sorted_vector
    (sizeof(struct model_callback),
     ALIGNOF(struct model_callback),
     compare_callbacks,
     app->allocator,
     &app->add_model_cbk_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_create_sorted_vector
    (sizeof(struct model_callback),
     ALIGNOF(struct model_callback),
     compare_callbacks,
     app->allocator,
     &app->remove_model_cbk_list);
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

  app_err = app_look_at
    (app,
     app->view,
     (float[]){10.f, 10.f, 10.f},
     (float[]){0.f, 0.f, 0.f},
     (float[]){0.f, 1.f, 0.f});
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
init_sys(struct app* app)
{
  struct sl_log_stream log_stream = { NULL, NULL };
  enum sl_error sl_err = SL_NO_ERROR;
  enum app_error app_err = APP_NO_ERROR;
  enum app_error tmp_err = APP_NO_ERROR;
  assert(app);

  sl_err = sl_create_logger(app->allocator, &app->logger);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }

  STATIC_ASSERT(sizeof(void*) >= 4, Unexpected_pointer_size);
  log_stream.data = (void*)0xDEADBEEF;
  log_stream.func = std_log_func;
  sl_err = sl_logger_add_stream(app->logger, &log_stream);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }

exit:
  return app_err;

error:
  tmp_err = shutdown_sys(app);
  assert(tmp_err == APP_NO_ERROR);
  goto exit;
}

static enum app_error
init(struct app* app, const char* graphic_driver)
{
  enum app_error app_err = APP_NO_ERROR;
  enum app_error tmp_err = APP_NO_ERROR;
  assert(app != NULL);

  app_err = init_sys(app);
  if(app_err != APP_NO_ERROR)
    goto error;

  app_err = init_window_manager(app);
  if(app_err !=  APP_NO_ERROR) {
    APP_LOG_ERR(app, "Error in intializing the window manager\n");
    goto error;
  }
  app_err = init_renderer(app, graphic_driver);
  if(app_err != APP_NO_ERROR) {
    APP_LOG_ERR(app, "Error in initializing the renderer\n");
    goto error;
  }
  app_err = init_resources(app);
  if(app_err != APP_NO_ERROR) {
    APP_LOG_ERR(app, "Error in initializing the resource module\n");
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
 * Core application functions.
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_init(struct app_args* args, struct app** out_app)
{
  struct app* app = NULL;
  struct app_model* mdl = NULL;
  struct app_model_instance* mdl_instance = NULL;
  struct mem_allocator* allocator = NULL;
  enum app_error app_err = APP_NO_ERROR;

  if(!args || !out_app) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  if(args->render_driver == NULL) {
    app_err = APP_RENDER_DRIVER_ERROR;
    goto error;
  }

  allocator = args->allocator ? args->allocator : &mem_default_allocator;
  app = MEM_CALLOC(allocator, 1, sizeof(struct app));
  if(app == NULL) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  app->allocator = allocator;

  app_err = init(app, args->render_driver);
  if(app_err != APP_NO_ERROR)
    goto error;

  if(args->model) {
    /* Create the model and add an instance of it to the world. */
   if(APP_NO_ERROR == app_create_model(app, args->model, &mdl)) {
      app_err = app_instantiate_model(app, mdl, &mdl_instance);
      if(app_err != APP_NO_ERROR)
        goto error;
      app_err = app_world_add_model_instances(app, app->world, 1, &mdl_instance);
      if(app_err != APP_NO_ERROR)
        goto error;
    }
  }

exit:
  if(out_app)
    *out_app = app;
  return app_err;

error:
  if(app) {
    UNUSED const enum app_error tmp_err = shutdown(app);
    assert(tmp_err == APP_NO_ERROR);
    MEM_FREE(allocator, app);
    app = NULL;
  }
  goto exit;
}

EXPORT_SYM enum app_error
app_shutdown(struct app* app)
{
  struct mem_allocator* allocator = NULL;
  enum app_error app_err = APP_NO_ERROR;

  if(!app) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  app_err = shutdown(app);
  if(app_err != APP_NO_ERROR)
    goto error;

  allocator = app->allocator;
  MEM_FREE(allocator, app);

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

EXPORT_SYM enum app_error
app_get_main_world(struct app* app, struct app_world** out_world)
{
  if(!app || !out_world)
    return APP_INVALID_ARGUMENT;
  *out_world = app->world;
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_get_model_list
  (struct app* app,
   size_t* len,
   struct app_model** model_list[])
{
  enum app_error app_err = APP_NO_ERROR;

  if(!app || !len || !model_list) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(sorted_vector_buffer
    (app->model_list, len, NULL, NULL, (void**)model_list));

exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_connect_model_callback
  (struct app* app,
   enum app_model_event event,
   void (*func)(struct app*, struct app_model*, void*),
   void* data)
{
  return manage_model_callback_list
    (app, event, func, data, CALLBACK_CONNECT);
}

EXPORT_SYM enum app_error
app_disconnect_model_callback
  (struct app* app,
   enum app_model_event event,
   void (*func)(struct app*, struct app_model*, void*),
   void* data)
{
  return manage_model_callback_list
    (app, event, func, data, CALLBACK_DISCONNECT);
}

/*******************************************************************************
 *
 * Private functions.
 *
 ******************************************************************************/
enum app_error
app_register_model(struct app* app, struct app_model* model)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  struct model_callback* cbk_list = NULL;
  size_t len = 0;
  size_t i = 0;

  if(!app || !model) {
    app_err = APP_NO_ERROR;
    goto error;
  }
  sl_err = sl_sorted_vector_insert(app->model_list, &model);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_sorted_vector_buffer
    (app->add_model_cbk_list, &len, NULL, NULL, (void**)&cbk_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  for(i = 0; i < len; ++i)
    cbk_list[i].func(app, model, cbk_list[i].data);

exit:
  return app_err;

error:
  goto exit;
}

enum app_error
app_unregister_model(struct app* app, struct app_model* model)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  struct model_callback* cbk_list = NULL;
  size_t len = 0;
  size_t i = 0;

  if(!app || !model) {
    app_err = APP_NO_ERROR;
    goto error;
  }
  sl_err = sl_sorted_vector_remove(app->model_list, &model);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_sorted_vector_buffer
    (app->remove_model_cbk_list, &len, NULL, NULL, (void**)&cbk_list);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  for(i = 0; i < len; ++i)
    cbk_list[i].func(app, model, cbk_list[i].data);

exit:
  return app_err;

error:
  goto exit;
}

enum app_error
app_is_model_registered
  (struct app* app,
   struct app_model* model,
   bool* is_registered)
{
  enum app_error app_err = APP_NO_ERROR;
  size_t i = 0;
  size_t len = 0;

  if(!app || !model || !is_registered) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(sorted_vector_find(app->model_list, &model, &i));
  SL(sorted_vector_buffer(app->model_list, &len, NULL, NULL, NULL));
  *is_registered = (i != len);

exit:
  return app_err;
error:
  goto exit;
}

enum app_error
app_register_model_instance
  (struct app* app,
   struct app_model_instance* instance)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  
  if(!app || !instance) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_sorted_vector_insert(app->model_instance_list, &instance);
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
app_unregister_model_instance
  (struct app* app,
   struct app_model_instance* instance)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app || !instance) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_sorted_vector_remove(app->model_instance_list, &instance);
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
  (struct app* app,
   struct app_model_instance* instance,
   bool* is_registered)
{
  enum app_error app_err = APP_NO_ERROR;
  size_t i = 0;
  size_t len = 0;

  if(!app || !instance || !is_registered) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(sorted_vector_find(app->model_instance_list, &instance, &i));
  SL(sorted_vector_buffer(app->model_instance_list, &len, NULL, NULL, NULL));
  *is_registered = (i != len);

exit:
  return app_err;
error:
  goto exit;
}

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
    case RSRC_ATTRIB_COLOR:
      attr_usage = RDR_ATTRIB_COLOR;
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

