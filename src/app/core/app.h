#ifndef APP_H
#define APP_H

#include "app/core/app_error.h"
#include "sys/sys.h"
#include <stdbool.h>
#include <stddef.h>

#ifndef NDEBUG
  #include <assert.h>
  #define APP(func) assert(APP_NO_ERROR == app_##func)
#else
  #define APP(func) app_##func
#endif /* NDEBUG */

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

extern enum app_error
app_init
  (struct app_args* args,
   struct app** app);

extern enum app_error
app_ref_get
  (struct app* app);

extern enum app_error
app_ref_put
  (struct app* app);

extern enum app_error
app_run
  (struct app* app,
   bool* keep_running);

extern enum app_error
app_log
  (struct app*,
   enum app_log_type log_type,
   const char* fmt,
   ...) FORMAT_PRINTF(3, 4);

/* Release all the created objects (model, instance, etc.). */
extern enum app_error
app_cleanup
  (struct app* app);

extern enum app_error
app_terminal_font
  (struct app* app,
   const char* path);

extern enum app_error
app_get_window_manager_device
  (struct app* app,
   struct wm_device** wm);

extern enum app_error
app_get_main_view
  (struct app* app,
   struct app_view** view);

extern enum app_error
app_get_main_world
  (struct app* app,
   struct app_world** world);

extern enum app_error
app_attach_callback
  (struct app* app,
   enum app_signal signal,
   app_callback_t callback,
   void* data);

extern enum app_error
app_detach_callback
  (struct app* app,
   enum app_signal signal,
   app_callback_t callback,
   void* data);

extern enum app_error
app_is_callback_attached
  (struct app* app,
   enum app_signal signal,
   app_callback_t callback,
   void* data,
   bool* is_attached);

/* When enabled, the terminal intercepts user input. */
extern enum app_error
app_enable_term
  (struct app* app,
   bool enable);

extern enum app_error
app_is_term_enabled
  (struct app* app,
   bool* is_enabled);

#endif /* APP_H */

