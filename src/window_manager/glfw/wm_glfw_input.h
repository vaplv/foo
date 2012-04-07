#ifndef WM_GLFW_INPUT_H
#define WM_GLFW_INPUT_H

#include "window_manager/wm_error.h"

struct wm_device;

extern enum wm_error
wm_init_inputs
  (struct wm_device* device);

extern enum wm_error
wm_shutdown_inputs
  (struct wm_device* device);

#endif /* WM_GLFW_INPUT_H */

