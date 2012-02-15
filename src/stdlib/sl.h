#ifndef SL_H
#define SL_H

#include "stdlib/sl_error.h"

#ifndef NDEBUG
  #include <assert.h>
  #define SL(func) assert(SL_NO_ERROR == sl_##func)
#else
  #define SL(func) sl_##func
#endif

#endif /* SL_H */

