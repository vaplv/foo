#ifndef SIMD_H
#define SIMD_H

#ifdef __SSE__
  #include "maths/simd/sse/sse.h"
#else
  #error unsupported_platform
#endif

#endif /* SIMD_H */

