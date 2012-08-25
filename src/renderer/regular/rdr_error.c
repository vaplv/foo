#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_system_c.h"
#include <stdarg.h>

/*******************************************************************************
 *
 * Error functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rdr_error
rdr_get_error_string(struct rdr_system* sys, const char** str_err)
{
  if(UNLIKELY(!sys || !str_err))
    return RDR_INVALID_ARGUMENT;
  *str_err = sys->errbuf_id == 0 ? NULL : sys->errbuf;
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_flush_error(struct rdr_system* sys)
{
  if(UNLIKELY(!sys))
    return RDR_INVALID_ARGUMENT;
  sys->errbuf_id = 0;
  sys->errbuf[0] = '\0';
  return RDR_NO_ERROR;
}

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
    default:
      rdr_err = RDR_UNKNOWN_ERROR;
      break;
  }
  return rdr_err;
}

enum rdr_error
rdr_print_error(struct rdr_system* sys, const char* fmt, ...)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(!sys || !fmt)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  if(sys->errbuf_id < RDR_ERRBUF_LEN - 1) {
    va_list vargs_list;
    const size_t size = RDR_ERRBUF_LEN - sys->errbuf_id - 1; /*-1<=>\0 char*/
    int i = 0;

    va_start(vargs_list, fmt);
    i = vsnprintf(sys->errbuf + sys->errbuf_id, size, fmt, vargs_list);
    assert(i > 0);
    if((size_t)i >= size) {
      sys->errbuf_id = RDR_ERRBUF_LEN;
      sys->errbuf[RDR_ERRBUF_LEN - 1] = '\0';
    } else {
      sys->errbuf_id += i;
    }
    va_end(vargs_list);
  }
exit:
  return rdr_err;
error:
  goto exit;
}

