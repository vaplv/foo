#ifndef WM_H
#define WM_H

#include "sys/sys.h"
#include "window_manager/wm_error.h"

#ifndef NDEBUG
  #include <assert.h>
  #define WM(func) assert(WM_NO_ERROR == wm_##func)
#else
  #define WM(func) wm_##func
#endif /* NDEBUG */

#if defined(BUILD_WM)
  #define WM_API EXPORT_SYM
#else
  #define WM_API IMPORT_SYM
#endif

#endif /* WM_H */

