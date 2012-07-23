#include "window_manager/glfw/wm_glfw_error.h"

enum wm_error
sl_to_wm_error(enum sl_error sl_err)
{
  enum wm_error wm_err = WM_NO_ERROR;
  switch(sl_err) {
    case SL_INVALID_ARGUMENT:
      wm_err = WM_INVALID_ARGUMENT;
      break;
    case SL_MEMORY_ERROR:
      wm_err = WM_MEMORY_ERROR;
      break;
    case SL_NO_ERROR:
      wm_err = WM_NO_ERROR;
      break;
    default:
      wm_err = WM_UNKNOWN_ERROR;
      break;
  }
  return wm_err;
}

