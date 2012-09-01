#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_system_c.h"
#include <stdarg.h>

/*******************************************************************************
 *
 * Private error functions.
 *
 ******************************************************************************/
enum rdr_error
sl_to_rdr_error(enum sl_error sl_err)
{
  enum rdr_error rdr_err = RDR_UNKNOWN_ERROR;

  switch(sl_err) {
    case SL_NO_ERROR:
      rdr_err = RDR_NO_ERROR;
      break;
    case SL_INVALID_ARGUMENT:
      rdr_err = RDR_INVALID_ARGUMENT;
      break;
    case SL_MEMORY_ERROR:
      rdr_err = RDR_MEMORY_ERROR;
      break;
    case SL_OVERFLOW_ERROR:
      rdr_err = RDR_OVERFLOW_ERROR;
      break;
    default:
      rdr_err = RDR_UNKNOWN_ERROR;
      break;
  }
  return rdr_err;
}
