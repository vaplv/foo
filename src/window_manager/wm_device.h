#ifndef WM_DEVICE_H
#define WM_DEVICE_H

#include "window_manager/wm.h"
#include "window_manager/wm_error.h"

struct mem_allocator;
struct wm_device;

WM_API enum wm_error
wm_create_device
  (struct mem_allocator* allocator, /* May be NULL. */
   struct wm_device** dev);

WM_API enum wm_error
wm_device_ref_get
  (struct wm_device* dev);

WM_API enum wm_error
wm_device_ref_put
  (struct wm_device* dev);

WM_API enum wm_error
wm_flush_events
  (struct wm_device* device);

#endif /* WM_DEVICE_H */

