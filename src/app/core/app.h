#ifndef APP_H
#define APP_H

#include "app/core/app_error.h"
#include <stdbool.h>
#include <stddef.h>

struct app_args {
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
  (struct app* app);

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
app_get_model_list
  (struct app* app,
   size_t* length,
   struct app_model** model_list[]);

extern enum app_error
app_get_model_instance_list
  (struct app* app,
   size_t* length,
   struct app_model_instance** model_instance_list[]);

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

#endif /* APP_H */

