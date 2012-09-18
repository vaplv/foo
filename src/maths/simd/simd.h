#ifndef SIMD_H
#define SIMD_H

#include "sys/sys.h"

#if defined(SIMD_BUILD_SHARED_LIBRARY)
  #define SIMD_API EXPORT_SYM
#elif defined(SIMD_USE_SHARED_LIBRARY)
  #define SIMD_API IMPORT_SYM
#else
  #define SIMD_API extern
#endif

#ifdef __SSE__
  #include "maths/simd/sse/sse.h"
#else
  #error unsupported_platform
#endif

#endif /* SIMD_H */

