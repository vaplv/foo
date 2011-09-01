#ifndef WM_H
#define WM_H

#include "window_manager/wm_types.h"

extern int 
wm_create_device
  (struct wm_device** dev);

extern int 
wm_free_device
  (struct wm_device* dev);

extern int 
wm_create_window
  (struct wm_device* dev, 
   const struct wm_window_desc* win_desc,
   struct wm_window** out_win);

extern int 
wm_free_window
  (struct wm_device* dev,
   struct wm_window* win);

extern int 
wm_swap
  (struct wm_device* dev,
   struct wm_window* win);

#endif /* WM_H */
