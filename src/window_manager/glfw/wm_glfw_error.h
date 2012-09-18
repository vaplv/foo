#ifndef WM_GLFW_ERROR_H
#define WM_GLFW_ERROR_H

#include "stdlib/sl_error.h"
#include "sys/sys.h"
#include "window_manager/wm_error.h"

LOCAL_SYM enum wm_error
sl_to_wm_error
  (enum sl_error sl_err);

#endif /* WM_GLFW_ERROR_H */

