#ifndef WM_DEVICE_H
#define WM_DEVICE_H

#include "window_manager/wm_error.h"

struct wm_device;

extern enum wm_error
wm_create_device
  (struct wm_device** dev);

extern enum wm_error 
wm_free_device
  (struct wm_device* dev);

extern enum wm_error
wm_flush_events
  (struct wm_device* device);

#endif /* WM_DEVICE_H */

