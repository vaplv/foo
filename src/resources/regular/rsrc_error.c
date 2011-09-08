#include "resources/regular/rsrc_error_c.h"

enum rsrc_error
sl_to_rsrc_error(enum sl_error sl_err)
{
  enum rsrc_error err = RSRC_UNKNOWN_ERROR;

  switch(sl_err) {
    case SL_NO_ERROR:
      err = RSRC_NO_ERROR;
      break;
    case SL_INVALID_ARGUMENT:
      err = RSRC_INVALID_ARGUMENT;
      break;
    case SL_MEMORY_ERROR:
      err = RSRC_MEMORY_ERROR;
      break;
    default:
      err = RSRC_UNKNOWN_ERROR;
      break;
  }

  return err;
}

