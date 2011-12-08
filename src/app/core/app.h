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
struct app_view;
struct app_world;
struct wm_device;
struct sys;

enum app_model_event {
  APP_MODEL_EVENT_ADD,
  APP_MODEL_EVENT_REMOVE
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
app_connect_model_callback
  (struct app* app,
   enum app_model_event event,
   void (*func)(struct app_model*, void*),
   void* data);

extern enum app_error
app_disconnect_model_callback
  (struct app* app,
   enum app_model_event event,
   void (*func)(struct app_model*, void*),
   void* data);

#endif /* APP_H */

