#ifndef SL_H
#define SL_H

#include "stdlib/sl_error.h"
#include "sys/sys.h"

#ifndef NDEBUG
  #include <assert.h>
  #define SL(func) assert(SL_NO_ERROR == sl_##func)
#else
  #define SL(func) sl_##func
#endif

#if defined(BUILD_SL)
  #define SL_API EXPORT_SYM
#else 
  #define SL_API IMPORT_SYM
#endif

#endif /* SL_H */

