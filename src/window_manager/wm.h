#ifndef WM_H
#define WM_H

#include "window_manager/wm_error.h"

#ifndef NDEBUG
  #include <assert.h>
  #define WM(func) assert(WM_NO_ERROR == wm_##func)
#else
  #define WM(func) wm_##func
#endif /* NDEBUG */

#endif /* WM_H */

