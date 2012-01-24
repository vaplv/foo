#ifndef RSRC_H
#define RSRC_H

#include "resources/rsrc_error.h"
#include <stddef.h>

struct rsrc_context;

#ifndef NDEBUG
  #include <assert.h>
  #define RSRC(func) assert(RSRC_NO_ERROR == rsrc_##func)
#else
  #define RSRC(func) rsrc_##func
#endif

enum rsrc_type {
  RSRC_FLOAT,
  RSRC_FLOAT2,
  RSRC_FLOAT3,
  RSRC_FLOAT4
};

/* Basic image saving function. */
extern enum rsrc_error
rsrc_write_ppm
  (struct rsrc_context* ctxt,
   const char* path,
   size_t width,
   size_t height,
   size_t bytes_per_pixel,
   const unsigned char* buffer);

#endif /* RSRC_H */

