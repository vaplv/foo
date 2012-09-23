#ifndef APP_H
#define APP_H

#include "sys/sys.h"

#ifndef NDEBUG
  #include <assert.h>
  #define APP(func) assert(APP_NO_ERROR == app_##func)
#else
  #define APP(func) app_##func
#endif /* NDEBUG */

#if defined(BUILD_APP)
  #define APP_API EXPORT_SYM
#else
  #define APP_API IMPORT_SYM
#endif

#endif /* APP_H */

