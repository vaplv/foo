#include "app/core/regular/app_c.h"
#include "app/core/regular/app_command_c.h"
#include "app/core/regular/app_cvar_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_term.h"
#include "app/core/regular/app_world_c.h"
#include "app/core/app.h"
#include "app/core/app_command.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "app/core/app_view.h"
#include "app/core/app_world.h"
#include "renderer/rdr.h"
#include "renderer/rdr_font.h"
#include "renderer/rdr_frame.h"
#include "renderer/rdr_material.h"
#include "renderer/rdr_system.h"
#include "renderer/rdr_term.h"
#include "renderer/rdr_world.h"
#include "resources/rsrc_context.h"
#include "resources/rsrc_font.h"
#include "resources/rsrc_geometry.h"
#include "resources/rsrc_wavefront_obj.h"
#include "stdlib/sl.h"
#include "stdlib/sl_flat_map.h"
#include "stdlib/sl_flat_set.h"
#include "stdlib/sl_logger.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "window_manager/wm.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_error.h"
#include "window_manager/wm_window.h"
#include <assert.h>
#include <error.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>

#define DEFAULT_WIN_WIDTH 800
#define DEFAULT_WIN_HEIGHT 600

static const wchar_t* default_charset =
  L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
  L" &~\"#'{([-|`_\\^@)]=}+$%*,?;.:/!<>";

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
struct callback {
  app_callback_t func;
  void* data;
};

enum callback_action {
  CALLBACK_ATTACH,
  CALLBACK_DETACH
};

static int
compare_callbacks(const void* a, const void* b)
{
  struct callback* cbk0 = (struct callback*)a;
  struct callback* cbk1 = (struct callback*)b;
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
  } else if(strncasecmp(msg, APP_WARN_PREFIX, sizeof(APP_WARN_PREFIX)-1) == 0) {
    fprintf(stdout, "\033[33m%s\033[0m", msg);
  } else {
    fprintf(stdout, "%s", msg);
  }
}

static void
term_log_func(const char* msg, void* data)
{
  wchar_t buffer[BUFSIZ];
  struct app* app = data;
  struct rdr_term* term = NULL;

  buffer[BUFSIZ - 1] = L'\0';
  assert(app);
  term = app->term.render_term;
  if(!term)
    return;

  if(strncasecmp(msg, APP_ERR_PREFIX, sizeof(APP_ERR_PREFIX)-1)==0) {
    msg += sizeof(APP_ERR_PREFIX) - 1;
    RDR(term_print_wstring
      (term, RDR_TERM_STDOUT, L"error: ", RDR_TERM_COLOR_RED));
  } else if(strncasecmp(msg, APP_WARN_PREFIX, sizeof(APP_WARN_PREFIX) - 1)==0) {
    msg += sizeof(APP_WARN_PREFIX) - 1;
    RDR(term_print_wstring
      (term, RDR_TERM_STDOUT, L"warning: ", RDR_TERM_COLOR_YELLOW));
  }
  mbstowcs(buffer, msg, BUFSIZ - 1);
  RDR(term_print_wstring
    (term, RDR_TERM_STDOUT, buffer, RDR_TERM_COLOR_WHITE));
}

static enum app_error
manage_callback
  (struct app* app,
   enum app_signal signal,
   app_callback_t cbk,
   void* data,
   enum callback_action action)
{
  struct sl_flat_set* set = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool is_updated = false;

  if(!app || !cbk) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  set = app->callback_list[signal];
  switch(action) {
    case CALLBACK_ATTACH:
      sl_err = sl_flat_set_insert
        (set, (struct callback[]){{cbk, data}}, NULL);
      break;
    case CALLBACK_DETACH:
      sl_err = sl_flat_set_erase
        (set, (struct callback[]){{cbk, data}}, NULL);
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
      case CALLBACK_ATTACH:
        SL(flat_set_erase(set, (struct callback[]){{cbk, data}}, NULL));
        break;
      case CALLBACK_DETACH:
        SL(flat_set_insert(set, (struct callback[]){{cbk, data}}, NULL));
        break;
      default:
        assert(false);
        break;
    }
  }
  goto exit;
}

static enum app_error
shutdown_window_manager(struct window_manager* wm, struct sl_logger* logger)
{
  enum app_error app_err = APP_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;
  assert(wm != NULL);

  if(wm->window) {
    wm_err = wm_window_ref_put(wm->window);
    if(wm_err != WM_NO_ERROR) {
      app_err = wm_to_app_error(wm_err);
      goto error;
    }
    wm->window = NULL;
  }
  if(wm->device) {
    wm_err = wm_device_ref_put(wm->device);
    if(wm_err != WM_NO_ERROR) {
      app_err = wm_to_app_error(wm_err);
      goto error;
    }
    wm->device = NULL;
  }
  if(MEM_IS_ALLOCATOR_VALID(&wm->allocator)) {
    if(MEM_ALLOCATED_SIZE(&wm->allocator)) {
      char dump[BUFSIZ];
      MEM_DUMP(&wm->allocator, dump, BUFSIZ);
      if(logger)
        APP_PRINT_MSG(logger, "Window manager leaks summary:\n%s\n", dump);
    }
    mem_shutdown_proxy_allocator(&wm->allocator);
    memset(&wm->allocator, 0, sizeof(struct mem_allocator));
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

  #define CALL(func) \
    do { \
      if((rdr_err = func) != RDR_NO_ERROR) { \
        app_err = rdr_to_app_error(rdr_err); \
        goto error; \
      } \
    } while(0)

  if(app->rdr.default_material) {
    CALL(rdr_material_ref_put(app->rdr.default_material));
    app->rdr.default_material = NULL;
  }
  if(app->rdr.term_font) {
    CALL(rdr_font_ref_put(app->rdr.term_font));
    app->rdr.term_font = NULL;
  }
  if(app->rdr.frame) {
    CALL(rdr_frame_ref_put(app->rdr.frame));
    app->rdr.frame = NULL;
  }
  if(app->rdr.system) {
    CALL(rdr_system_ref_put(app->rdr.system));
    app->rdr.system = NULL;
  }
  #undef CALL

  if(MEM_IS_ALLOCATOR_VALID(&app->rdr.allocator)) {
    if(MEM_ALLOCATED_SIZE(&app->rdr.allocator)) {
      char dump[BUFSIZ];
      MEM_DUMP(&app->rdr.allocator, dump, BUFSIZ);
      if(app->logger)
        APP_PRINT_MSG(app->logger, "Renderer leaks summary:\n%s\n", dump);
    }
    mem_shutdown_proxy_allocator(&app->rdr.allocator);
    memset(&app->rdr.allocator, 0, sizeof(struct mem_allocator));
  }

exit:
  return app_err;
error:
  goto exit;
}

static enum app_error
shutdown_resources(struct resources* rsrc, struct sl_logger* logger)
{
  enum app_error app_err = APP_NO_ERROR;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  assert(rsrc != NULL);

  #define CALL(func) \
    do { \
      if(RSRC_NO_ERROR != (rsrc_err = func)) { \
        app_err = rsrc_to_app_error(rsrc_err); \
        goto error; \
      } \
    } while(0)

  if(rsrc->wavefront_obj) {
    CALL(rsrc_wavefront_obj_ref_put(rsrc->wavefront_obj));
    rsrc->wavefront_obj = NULL;
  }
  if(rsrc->context) {
    CALL(rsrc_context_ref_put(rsrc->context));
    rsrc->context = NULL;
  }
  if(rsrc->font) {
    CALL(rsrc_font_ref_put(rsrc->font));
    rsrc->font = NULL;
  }
  #undef CALL

  if(MEM_IS_ALLOCATOR_VALID(&rsrc->allocator)) {
    if(MEM_ALLOCATED_SIZE(&rsrc->allocator)) {
      char dump[BUFSIZ];
      MEM_DUMP(&rsrc->allocator, dump, BUFSIZ);
      if(logger)
        APP_PRINT_MSG(logger, "Resource leaks summary:\n%s\n", dump);
    }
    mem_shutdown_proxy_allocator(&rsrc->allocator);
    memset(&rsrc->allocator, 0, sizeof(struct mem_allocator));
  }

exit:
  return app_err;
error:
  goto exit;
}

static enum app_error
shutdown_common(struct app* app)
{
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;
  assert(app != NULL);

  for(i = 0; i < APP_NB_SIGNALS; ++i) {
    if(NULL != app->callback_list[i]) {
      SL(free_flat_set(app->callback_list[i]));
      app->callback_list[i] = NULL;
    }
  }
  if(app->world)
    APP(world_ref_put(app->world));
  if(app->view)
    APP(view_ref_put(app->view));
  return app_err;
}

static enum app_error
shutdown_sys(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;

  assert(app);

  if(app->logger) {
    const enum sl_error sl_err = sl_free_logger(app->logger);
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
    #define CALL(func) if((app_err = func) != APP_NO_ERROR) goto error
    CALL(app_shutdown_cvar_system(app));
    CALL(app_shutdown_command_system(app));
    CALL(app_shutdown_term(app));
    CALL(app_shutdown_object_system(app));
    CALL(shutdown_common(app));
    CALL(shutdown_resources(&app->rsrc, app->logger));
    CALL(shutdown_renderer(app));
    CALL(shutdown_window_manager(&app->wm, app->logger));
    CALL(shutdown_sys(app));
    #undef CALL
  }
exit:
  return app_err;
error:
  goto exit;
}

static enum app_error
init_window_manager(struct window_manager* wm)
{
  const struct wm_window_desc win_desc = {
    .width = DEFAULT_WIN_WIDTH,
    .height = DEFAULT_WIN_HEIGHT,
    .fullscreen = false
  };
  enum app_error app_err = APP_NO_ERROR;
  enum app_error tmp_err = APP_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;
  assert(wm != NULL);

  mem_init_proxy_allocator
    ("window manager:", &wm->allocator, &mem_default_allocator);
  wm_err = wm_create_device(&wm->allocator, &wm->device);
  if(wm_err != WM_NO_ERROR) {
    app_err = wm_to_app_error(wm_err);
    goto error;
  }
  wm_err = wm_create_window(wm->device, &win_desc, &wm->window);
  if(wm_err != WM_NO_ERROR) {
    app_err = wm_to_app_error(wm_err);
    goto error;
  }

exit:
  return app_err;
error:
  tmp_err = shutdown_window_manager(wm, NULL);
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
    ("renderer", &app->rdr.allocator, &mem_default_allocator);
  #define CALL(func) \
    do { \
      if((rdr_err = func) != RDR_NO_ERROR) { \
        app_err = rdr_to_app_error(rdr_err); \
        goto error; \
      } \
    } while(0)
  CALL(rdr_create_system(driver, &app->rdr.allocator, &app->rdr.system));
  CALL(rdr_system_attach_log_stream(app->rdr.system, &term_log_func, app));
  CALL(rdr_system_attach_log_stream
    (app->rdr.system, &std_log_func, (void*)(0xDEADBEEF)));

  CALL(rdr_create_frame(app->rdr.system, &app->rdr.frame));
  CALL(rdr_background_color(app->rdr.frame, (float[]){0.1f, 0.1f, 0.1f}));
  CALL(rdr_create_material(app->rdr.system, &app->rdr.default_material));
  CALL(rdr_create_font(app->rdr.system, &app->rdr.term_font));

  rdr_err = rdr_material_program
    (app->rdr.default_material, default_shader_sources);
  if(rdr_err != RDR_NO_ERROR) {
    const char* log = NULL;
    RDR(get_material_log(app->rdr.default_material, &log));
    if(log  != NULL)
      APP_PRINT_ERR(app->logger, "Default render material error: \n%s", log);
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }
  #undef CALL

exit:
  return app_err;
error:
  tmp_err = shutdown_renderer(app);
  assert(tmp_err == APP_NO_ERROR);
  goto exit;
}

static enum app_error
init_resources(struct resources* rsrc)
{
  enum app_error app_err = APP_NO_ERROR;
  enum app_error tmp_err = APP_NO_ERROR;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  assert(rsrc != NULL);

  mem_init_proxy_allocator
    ("resources", &rsrc->allocator, &mem_default_allocator);

  #define CALL(func) \
    do { \
      if((rsrc_err = func) != RSRC_NO_ERROR) { \
        app_err = rsrc_to_app_error(rsrc_err); \
        goto error; \
      } \
    } while(0)
  CALL(rsrc_create_context(&rsrc->allocator, &rsrc->context));
  CALL(rsrc_create_wavefront_obj(rsrc->context, &rsrc->wavefront_obj));
  CALL(rsrc_create_font(rsrc->context, NULL, &rsrc->font));
  #undef CALL
exit:
  return app_err;
error:
  tmp_err = shutdown_resources(rsrc, NULL);
  assert(tmp_err == APP_NO_ERROR);
  goto exit;
}

static enum app_error
init_common(struct app* app)
{
  int i = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum app_error tmp_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  assert(app != NULL);

  #define CALL(func) \
    do { \
      if(APP_NO_ERROR != (app_err = func)) \
        goto error; \
    } while(0)

  for(i = 0; i < APP_NB_SIGNALS; ++i) {
    sl_err = sl_create_flat_set
      (sizeof(struct callback),
       ALIGNOF(struct callback),
       compare_callbacks,
       app->allocator,
       &app->callback_list[i]);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
  }
  CALL(app_create_world(app, &app->world));
  CALL(app_create_view(app, &app->view));
  CALL(app_look_at
    (app->view,
     (float[]){10.f, 10.f, 10.f},
     (float[]){0.f, 0.f, 0.f},
     (float[]){0.f, 1.f, 0.f}));

  #undef CALL

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

  #define CALL(func) \
    do { \
      if(SL_NO_ERROR != (sl_err = func)) { \
        app_err = sl_to_app_error(sl_err); \
        goto error; \
      } \
    } while(0)

  CALL(sl_create_logger(app->allocator, &app->logger));

  STATIC_ASSERT(sizeof(void*) >= 4, Unexpected_pointer_size);
  log_stream.data = (void*)0xDEADBEEF;
  log_stream.func = std_log_func;
  CALL(sl_logger_add_stream(app->logger, &log_stream));

  log_stream.data = app;
  log_stream.func = term_log_func;
  CALL(sl_logger_add_stream(app->logger, &log_stream));

  #undef CALL
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

  #define CALL(func, err_msg) \
    do { \
      if(APP_NO_ERROR != (app_err = func)) { \
        if(err_msg[0] != '\0') { \
          APP_PRINT_ERR(app->logger, err_msg); \
        } \
        goto error; \
      } \
    } while(0)
  CALL(init_window_manager(&app->wm), "error initializing, window manager\n");
  CALL(init_renderer(app, graphic_driver), "error initializing renderer\n");
  CALL(init_resources(&app->rsrc), "error initializing resource module\n");
  CALL(init_common(app), "");
  CALL(app_init_object_system(app), "error initializing object system\n");
  CALL(app_init_term(app), "error initializing terminal\n");
  CALL(app_init_command_system(app), "error intializing command system\n");
  CALL(app_init_cvar_system(app), "error intializing cvar system\n");
  #undef CALL

exit:
  return app_err;
error:
  tmp_err = shutdown(app);
  assert(tmp_err == APP_NO_ERROR);
  goto exit;
}

static void
release_app(struct ref* ref)
{
  struct app* app = NULL;
  enum app_error app_err = APP_NO_ERROR;
  assert(ref != NULL);

  app = CONTAINER_OF(ref, struct app, ref);
  app_err = shutdown(app);
  assert(app_err == APP_NO_ERROR);
  MEM_FREE(app->allocator, app);
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
  ref_init(&app->ref);

  app_err = init(app, args->render_driver);
  if(app_err != APP_NO_ERROR)
    goto error;

  if(args->term_font) {
    app_err = app_terminal_font(app, args->term_font);
    if(app_err != APP_NO_ERROR)
      goto error;
  }

  if(args->model) {
    /* Create the model and add an instance of it to the world. */
    if(APP_NO_ERROR == app_create_model(app, args->model, NULL, &mdl)) {
      app_err = app_instantiate_model(app, mdl, NULL, &mdl_instance);
      if(app_err != APP_NO_ERROR)
        goto error;
      app_err = app_world_add_model_instances(app->world, 1, &mdl_instance);
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
    APP(cleanup(app));
    UNUSED const enum app_error tmp_err = shutdown(app);
    assert(tmp_err == APP_NO_ERROR);
    MEM_FREE(allocator, app);
    app = NULL;
  }
  goto exit;
}

EXPORT_SYM enum app_error
app_ref_get(struct app* app)
{
  if(!app)
    return APP_INVALID_ARGUMENT;
  ref_get(&app->ref);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_ref_put(struct app* app)
{
  if(!app)
    return APP_INVALID_ARGUMENT;
  ref_put(&app->ref, release_app);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_run(struct app* app, bool* keep_running)
{
  enum app_error app_err = APP_NO_ERROR;

  if(!app || !keep_running) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  app_err = app_draw_world(app->world, app->view);
  if(app_err != APP_NO_ERROR)
    goto error;

  if(app->term.is_enabled)
    RDR(frame_draw_term(app->rdr.frame, app->term.render_term));

  RDR(flush_frame(app->rdr.frame));
  WM(swap(app->wm.window));
  *keep_running = !app->post_exit;
  app->post_exit = false;

exit:
  return app_err;

error:
  goto exit;
}

EXPORT_SYM enum app_error
app_flush_events(struct app* app)
{
  enum wm_error wm_err = WM_NO_ERROR;
  wm_err = wm_flush_events(app->wm.device);
  return wm_to_app_error(wm_err);
}

EXPORT_SYM enum app_error
app_log(struct app* app, enum app_log_type type, const char* fmt, ...)
{
  va_list vargs_list;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  char buf[128];
  int err = 0;

  if(!app || type == APP_NB_LOG_TYPES || !fmt) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  if(strlen(fmt) + 1 > sizeof(buf)/sizeof(char)) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  va_start(vargs_list, fmt);
  switch(type) {
    case APP_LOG_ERROR:
      err = sprintf(buf, "%s%s", APP_ERR_PREFIX, fmt);
      if(err < 0) {
        app_err = APP_INTERNAL_ERROR;
        goto error;
      } else {
        sl_err = sl_logger_vprint(app->logger, buf, vargs_list);
        if(sl_err != SL_NO_ERROR) {
          app_err = sl_to_app_error(sl_err);
          goto error;
        }
      }
      break;
    case APP_LOG_INFO:
      sl_err = sl_logger_vprint(app->logger, fmt, vargs_list);
      break;
    case APP_LOG_WARNING:
      err = sprintf(buf, "%s%s", APP_WARN_PREFIX, fmt);
      if(err < 0) {
        app_err = APP_INTERNAL_ERROR;
        goto error;
      } else {
        sl_err = sl_logger_vprint(app->logger, buf, vargs_list);
        if(sl_err != SL_NO_ERROR) {
          app_err = sl_to_app_error(sl_err);
          goto error;
        }
      }
      break;
    default: assert(0); break;
  }
  va_end(vargs_list);

exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_cleanup(struct app* app)
{
  return app_clear_object_system(app);
}

EXPORT_SYM enum app_error
app_terminal_font(struct app* app, const char* path)
{
  struct rdr_glyph_desc* glyph_desc_list = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  size_t nb_chars = 0;
  size_t i = 0;

  if(!app || !path) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  rsrc_err = rsrc_load_font(app->rsrc.font, path);
  if(RSRC_NO_ERROR != rsrc_err) {
    APP_PRINT_ERR(app->logger, "Error loading font `%s'.\n", path);
    app_err = rsrc_to_app_error(rsrc_err);
    goto error;
  }

  nb_chars = wcslen(default_charset);
  if(nb_chars) {
    size_t line_space = 0;
    enum rdr_error rdr_err = RDR_NO_ERROR;

    glyph_desc_list = MEM_CALLOC
      (&app->rdr.allocator, nb_chars, sizeof(struct rdr_glyph_desc));
    for(i = 0; i < nb_chars; ++i) {
      struct rsrc_glyph_desc glyph_desc;
      struct rsrc_glyph* glyph = NULL;
      size_t width = 0;
      size_t height = 0;
      size_t Bpp = 0;

      rsrc_err = rsrc_font_glyph(app->rsrc.font, default_charset[i], &glyph);
      if(RSRC_NO_ERROR != rsrc_err) {
        APP_PRINT_WARN
          (app->logger,
           "Character `%c' not found in font `%s'.\n",
           default_charset[i],
           path);
      } else {
        size_t size = 0;
        /* Get glyph desc. */
        RSRC(glyph_desc(glyph, &glyph_desc));
        glyph_desc_list[i].width = glyph_desc.width;
        glyph_desc_list[i].character = glyph_desc.character;
        glyph_desc_list[i].bitmap_left = glyph_desc.bbox.x_min;
        glyph_desc_list[i].bitmap_top = glyph_desc.bbox.y_min;
        /* Get glyph bitmap. */
        RSRC(glyph_bitmap(glyph, true, &width, &height, &Bpp, NULL));
        glyph_desc_list[i].bitmap.width = width;
        glyph_desc_list[i].bitmap.height = height;
        glyph_desc_list[i].bitmap.bytes_per_pixel = Bpp;
        glyph_desc_list[i].bitmap.buffer = NULL;
        size = width * height * Bpp;
        if(size) {
          unsigned char* buffer = MEM_CALLOC(&app->rdr.allocator, 1, size);
          if(!buffer) {
            app_err = APP_MEMORY_ERROR;
            goto error;
          }
          RSRC(glyph_bitmap(glyph, true, NULL, NULL, NULL, buffer));
          glyph_desc_list[i].bitmap.buffer = buffer;
        }
        RSRC(glyph_ref_put(glyph));
      }
    }
    RSRC(font_line_space(app->rsrc.font, &line_space));

    rdr_err = rdr_font_data
      (app->rdr.term_font, line_space, nb_chars, glyph_desc_list);

    if(RDR_NO_ERROR != rdr_err) {
      APP_PRINT_ERR
        (app->logger,
         "Error setting-up render data of the font `%s'.\n",
         path);
      app_err = rdr_to_app_error(rdr_err);
      goto error;
    }
  }
exit:
  if(glyph_desc_list) {
    nb_chars =  wcslen(default_charset);
    for(i = 0; i < nb_chars; ++i)
      MEM_FREE(&app->rdr.allocator, glyph_desc_list[i].bitmap.buffer);
    MEM_FREE(&app->rdr.allocator, glyph_desc_list);
  }
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_get_window_manager_device(struct app* app, struct wm_device** out_wm)
{
  if(!app || !out_wm)
    return APP_INVALID_ARGUMENT;
  *out_wm = app->wm.device;
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
app_is_callback_attached
  (struct app* app,
   enum app_signal signal,
   app_callback_t callback,
   void* data,
   bool* is_attached)
{
  struct sl_flat_set* set = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  size_t i = 0;
  size_t len = 0;

  if(!app || !callback || !is_attached || signal >= APP_NB_SIGNALS) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  set = app->callback_list[signal];
  sl_err = sl_flat_set_find
    (set, (struct callback[]){{callback, data}}, &i);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  sl_err = sl_flat_set_buffer(set, &len, NULL, NULL, NULL);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  *is_attached = (i != len);

exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_attach_callback
  (struct app* app,
   enum app_signal signal,
   app_callback_t callback,
   void* data)
{
  return manage_callback
    (app, signal, callback, data, CALLBACK_ATTACH);
}

EXPORT_SYM enum app_error
app_detach_callback
  (struct app* app,
   enum app_signal signal,
   app_callback_t callback,
   void* data)
{
  return manage_callback
    (app, signal, callback, data, CALLBACK_DETACH);
}

EXPORT_SYM enum app_error
app_enable_term(struct app* app, bool enable)
{
  if(!app)
    return APP_INVALID_ARGUMENT;
  return app_setup_term(app, enable);
}

EXPORT_SYM enum app_error
app_is_term_enabled(struct app* app, bool* is_enabled)
{
  if(!app || !is_enabled)
    return APP_INVALID_ARGUMENT;
  *is_enabled = app->term.is_enabled;
  return APP_NO_ERROR;
}

/*******************************************************************************
 *
 * Private functions.
 *
 ******************************************************************************/
enum app_error
app_mapped_name_completion
  (struct sl_flat_map* map,
   const char* name,
   size_t name_len,
   size_t* completion_list_len,
   const char** completion_list[])
{
  const char** name_list = NULL;
  size_t len = 0;
  enum app_error app_err = APP_NO_ERROR;

  if(!map
  || (name_len && !name)
  || !completion_list_len
  || !completion_list) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(flat_map_key_buffer(map, &len, NULL, NULL, (void**)&name_list));
  if(0 == name_len) {
    *completion_list_len = len;
    *completion_list = name_list;
  } else {
    #define CHARBUF_SIZE 256
    char buf[CHARBUF_SIZE];
    const char* ptr = buf;
    size_t begin = 0;
    size_t end = 0;

    /* Copy the name in a mutable buffer. */
    if(name_len > CHARBUF_SIZE - 1) {
      app_err = APP_MEMORY_ERROR;
      goto error;
    }
    strncpy(buf, name, name_len);
    buf[name_len] = '\0';
    /* Find the completion range. */
    SL(flat_map_lower_bound(map, &ptr, &begin));
    buf[name_len] = 127;
    buf[name_len + 1] = '\0';
    SL(flat_map_upper_bound(map, &ptr, &end));
    /* Define the completion list. */
    *completion_list = name_list + begin;
    *completion_list_len = (name_list + end) - (*completion_list);
    if(0 == *completion_list_len)
      *completion_list = NULL;
    #undef CHARBUF_SIZE
  }
exit:
  return app_err;
error:
  goto exit;
}

enum app_error
app_invoke_callbacks(struct app* app, enum app_signal signal, ...)
{
  va_list arg_list;
  void* arg = NULL;
  struct callback* callback_list = NULL;
  size_t len = 0;
  size_t i = 0;
  enum app_error app_err = APP_NO_ERROR;

  if(!app || signal >= APP_NB_SIGNALS) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  va_start(arg_list, signal);
  arg = va_arg(arg_list, void*);

  SL(flat_set_buffer
    (app->callback_list[signal], &len, NULL, NULL, (void**)&callback_list));

  for(i = 0; i < len; ++i) {
    app_callback_t func = callback_list[i].func;
    void* data = callback_list[i].data;

    switch(signal) {
      case APP_SIGNAL_DESTROY_MODEL:
      case APP_SIGNAL_CREATE_MODEL:
        ((void (*)(struct app_model*, void*))func)(arg, data);
        break;
      case APP_SIGNAL_CREATE_MODEL_INSTANCE:
      case APP_SIGNAL_DESTROY_MODEL_INSTANCE:
        ((void (*)(struct app_model_instance*, void*))func)(arg, data);
        break;
      default:
        assert(false);
        break;
    }
  }
  va_end(arg_list);

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

