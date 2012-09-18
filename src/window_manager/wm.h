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

#if defined(WM_BUILD_SHARED_LIBRARY)
  #define WM_API EXPORT_SYM
#elif defined(WM_USE_SHARED_LIBRARY)
  #define WM_API IMPORT_SYM
#else
  #define WM_API extern
#endif

#endif /* WM_H */

