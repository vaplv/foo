#ifndef WM_WINDOW_H
#define WM_WINDOW_H

#include "window_manager/wm.h"
#include "window_manager/wm_error.h"
#include <stdbool.h>

struct wm_window_desc {
  unsigned int width;
  unsigned int height;
  bool fullscreen;
};

struct wm_window;

WM_API enum wm_error 
wm_create_window
  (struct wm_device* dev, 
   const struct wm_window_desc* win_desc,
   struct wm_window** out_win);

WM_API enum wm_error 
wm_window_ref_get
  (struct wm_window* win);

WM_API enum wm_error
wm_window_ref_put
  (struct wm_window* win);

WM_API enum wm_error 
wm_swap
  (struct wm_window* win);

WM_API enum wm_error
wm_get_window_desc
  (struct wm_window* win,
   struct wm_window_desc* win_desc);

#endif /* WM_WINDOW_H */

