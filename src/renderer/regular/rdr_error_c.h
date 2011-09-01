#ifndef RDR_ERROR_C_H
#define RDR_ERROR_C_H

#include "renderer/rdr_error.h"
#include "stdlib/sl_error.h"

extern enum rdr_error
sl_to_rdr_error
  (enum sl_error sl_err);

#endif /* RDR_ERROR_C_H */

