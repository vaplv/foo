#ifndef RDR_SYSTEM_C_H
#define RDR_SYSTEM_C_H

#include "render_backend/rbi.h"

#ifndef NDEBUG
  #define SL(func) assert(SL_NO_ERROR == sl_##func)
#else
  #define SL(func) sl_##func
#endif

struct rb_context;
struct sl_context;

struct rdr_system {
  /* The render backend and its context. */
  struct rbi rb; 
  struct rb_context* ctxt;
  /* The context of the standard library. */
  struct sl_context* sl_ctxt;
};

#endif /* RDR_SYSTEM_C_H. */

