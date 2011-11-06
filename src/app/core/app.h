#ifndef APP_H
#define APP_H

#include "app/core/app_error.h"
#include <stdbool.h>

struct app_args {
  const char* render_driver;
  const char* model;
};

struct app;
struct app_view;
struct wm_device;
struct sys;

extern enum app_error
app_init
  (struct app_args* args,
   struct app** app);

extern enum app_error
app_shutdown
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

#endif /* APP_H */

