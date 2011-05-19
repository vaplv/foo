#ifndef WM_H
#define WM_H

#include "window_manager/wm_types.h"

extern int wm_create_device(struct wm_device**);
extern int wm_free_device(struct wm_device*);
extern int wm_create_window
  (struct wm_device*, struct wm_window**, struct wm_window_desc*);
extern int wm_free_window(struct wm_device*, struct wm_window*);
extern int wm_swap(struct wm_device*, struct wm_window*);

#endif /* WM_H */
