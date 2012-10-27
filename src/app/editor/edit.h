#ifndef EDIT_H
#define EDIT_H

#include "sys/sys.h"

#if defined(BUILD_EDIT)
  #define EDIT_API EXPORT_SYM
#else
  #define EDIT_API IMPORT_SYM
#endif

#ifndef NDEBUG
  #include <assert.h>
  #define EDIT(func) assert(EDIT_NO_ERROR == edit_##func)
#else
  #define EDIT(func) edit_##func
#endif /* NDEBUG */

#endif /* EDIT_H */

