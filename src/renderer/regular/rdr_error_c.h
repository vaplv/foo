#ifndef RDR_ERROR_C_H
#define RDR_ERROR_C_H

#include "renderer/rdr_error.h"
#include "stdlib/sl_error.h"
#include "sys/sys.h"

struct rdr_system;

LOCAL_SYM enum rdr_error
sl_to_rdr_error
  (enum sl_error sl_err);

LOCAL_SYM enum rdr_error
rdr_print_error
  (struct rdr_system* sys,
   const char* fmt,
   ...) FORMAT_PRINTF(2, 3);

#endif /* RDR_ERROR_C_H */

