#ifndef RDR_H
#define RDR_H

#include "sys/sys.h"
#include "renderer/rdr_error.h"

#ifndef NDEBUG
  #include <assert.h>
  #define RDR(func) assert(RDR_NO_ERROR == rdr_##func)
#else
  #define RDR(func) rdr_##func
#endif

#if defined(BUILD_RDR)
  #define RDR_API EXPORT_SYM
#else
  #define RDR_API IMPORT_SYM
#endif

#endif /* RDR_H */

