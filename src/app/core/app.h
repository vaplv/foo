#ifndef APP_H
#define APP_H

#include "sys/sys.h"

#ifndef NDEBUG
  #include <assert.h>
  #define APP(func) assert(APP_NO_ERROR == app_##func)
#else
  #define APP(func) app_##func
#endif /* NDEBUG */

#if defined(APP_BUILD_SHARED_LIBRARY)
  #define APP_API EXPORT_SYM
#elif defined(APP_USE_SHARED_LIBRARY)
  #define APP_API IMPORT_SYM
#else
  #define APP_API extern
#endif

#endif /* APP_H */

