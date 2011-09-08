#ifndef RSRC_ERROR_C_H
#define RSRC_ERROR_C_H

#include "resources/rsrc_error.h"
#include "stdlib/sl_error.h"

extern enum rsrc_error
sl_to_rsrc_error
  (enum sl_error sl_err);

#endif /* RSRC_ERROR_C_H */

