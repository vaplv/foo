#ifndef RDR_SYSTEM_C_H
#define RDR_SYSTEM_C_H

#include "render_backend/rbi.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"

#ifndef NDEBUG
  #include <assert.h>
  #define SL(func) assert(SL_NO_ERROR == sl_##func)
#else
  #define SL(func) sl_##func
#endif

struct rb_context;

struct rdr_system {
  struct mem_allocator* allocator;
  struct ref ref;

  /* The render backend and its context. */
  struct rbi rb; 
  struct rb_context* ctxt;
};

#endif /* RDR_SYSTEM_C_H. */

