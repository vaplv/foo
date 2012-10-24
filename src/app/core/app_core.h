#ifndef APP_CORE_H
#define APP_CORE_H

#include "app/core/app_pick_id.h"
#include "app/core/app_error.h"
#include <stdbool.h>
#include <stddef.h>

struct app_args {
  const char* term_font;
  const char* render_driver;
  const char* model;
  struct mem_allocator* allocator; /* May be NULL. */
};

struct app;
struct app_model;
struct app_model_instance;
struct app_view;
struct app_world;
struct wm_device;
struct wm_window;
struct sys;

typedef void(*app_callback_t)(void);
#define APP_CALLBACK(v) (app_callback_t)(v)

enum app_signal {
  APP_SIGNAL_CREATE_MODEL,
  APP_SIGNAL_DESTROY_MODEL,
  APP_SIGNAL_CREATE_MODEL_INSTANCE,
  APP_SIGNAL_DESTROY_MODEL_INSTANCE,
  APP_NB_SIGNALS
};

enum app_log_type {
  APP_LOG_ERROR,
  APP_LOG_INFO,
  APP_LOG_WARNING,
  APP_NB_LOG_TYPES
};

APP_API enum app_error
app_init
  (struct app_args* args,
   struct app** app);

APP_API enum app_error
app_ref_get
  (struct app* app);

APP_API enum app_error
app_ref_put
  (struct app* app);

APP_API enum app_error
app_run
  (struct app* app,
   bool* keep_running);

APP_API enum app_error
app_flush_events
  (struct app* app);

APP_API enum app_error
app_log
  (struct app*,
   enum app_log_type log_type,
   const char* fmt,
   ...) FORMAT_PRINTF(3, 4);

/* Release all the created objects (model, instance, etc.). */
APP_API enum app_error
app_cleanup
  (struct app* app);

APP_API enum app_error
app_terminal_font
  (struct app* app,
   const char* path);

APP_API enum app_error
app_get_window_manager_device
  (struct app* app,
   struct wm_device** wm);

APP_API enum app_error
app_get_active_window
  (struct app* app,
   struct wm_window** window); /* May be NULL. */

APP_API enum app_error
app_get_main_view
  (struct app* app,
   struct app_view** view);

APP_API enum app_error
app_get_main_world
  (struct app* app,
   struct app_world** world);

APP_API enum app_error
app_attach_callback
  (struct app* app,
   enum app_signal signal,
   app_callback_t callback,
   void* data);

APP_API enum app_error
app_detach_callback
  (struct app* app,
   enum app_signal signal,
   app_callback_t callback,
   void* data);

APP_API enum app_error
app_is_callback_attached
  (struct app* app,
   enum app_signal signal,
   app_callback_t callback,
   void* data,
   bool* is_attached);

/* When enabled, the terminal intercepts user inputs. */
APP_API enum app_error
app_enable_term
  (struct app* app,
   bool enable);

APP_API enum app_error
app_is_term_enabled
  (struct app* app,
   bool* is_enabled);

APP_API enum app_error
app_clear_picking
  (struct app* app);

/* Retrieve the picked id of the main world */
APP_API enum app_error
app_poll_picking
  (struct app* app,
   size_t* count,
   const app_pick_t* model_instance_id_list[]);

#endif /* APP_CORE_H */

