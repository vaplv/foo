#ifndef SL_CONTEXT_H
#define SL_CONTEXT_H

#include "stdlib/sl_error.h"

struct sl_context;

extern enum sl_error
sl_create_context
  (struct sl_context** out_ctxt);

extern enum sl_error
sl_free_context
  (struct sl_context* ctxt);

#endif /* SL_CONTEXT_H */
