#ifndef SIMD_H
#define SIMD_H

#include "sys/sys.h"

#if defined(BUILD_SIMD)
  #define SIMD_API EXPORT_SYM
#else
  #define SIMD_API IMPORT_SYM
#endif

#ifdef __SSE__
  #include "maths/simd/sse/sse.h"
#else
  #error unsupported_platform
#endif

#endif /* SIMD_H */

