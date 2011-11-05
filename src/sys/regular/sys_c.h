#ifndef SYS_C_H
#define SYS_C_H

#include "stdlib/sl_error.h"
#include "sys/sys_error.h"

#ifndef NDEBUG
  #include <assert.h>
  #define SL(func) assert(SL_NO_ERROR == sl_##func)
#else
  #define SL(func) sl_##func
#endif

struct sl_context;
struct sys;

struct sys_context {
  struct sl_context* sl_ctxt;
};

extern struct sys_context*
sys_context
  (struct sys* sys);

extern enum sys_error 
sl_to_sys_error
  (enum sl_error sl_err);

#endif /* SYS_C_H */

