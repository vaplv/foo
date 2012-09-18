#include "app/core/regular/app_error_c.h"
#include "sys/sys.h"
#include <stdlib.h>

/*******************************************************************************
 *
 * Error public functions.
 *
 ******************************************************************************/
const char*
app_error_string(enum app_error err)
{
  const char* out_str = NULL;
  switch(err) {
    case APP_ALIGNMENT_ERROR:
      out_str = "alignment error";
      break;
    case APP_COMMAND_ERROR:
      out_str = "command error";
      break;
    case APP_INTERNAL_ERROR:
      out_str = "internal error";
      break;
    case APP_INVALID_ARGUMENT:
      out_str = "invalid argument";
      break;
    case APP_INVALID_FILE:
      out_str = "invalid file";
      break;
    case APP_IO_ERROR:
      out_str = "input/output error";
      break;
    case APP_MEMORY_ERROR:
      out_str = "memory error";
      break;
    case APP_NO_ERROR:
      out_str = "no error";
      break;
    case APP_RENDER_DRIVER_ERROR:
      out_str = "render driver error";
      break;
    case APP_UNKNOWN_ERROR:
      out_str = "unknown error";
      break;
    default:
      out_str = "none";
      break;
  }
  return out_str;
}

/*******************************************************************************
 *
 * Error private functions.
 *
 ******************************************************************************/
enum app_error
sl_to_app_error(enum sl_error err)
{
  enum app_error app_err = APP_NO_ERROR;
  switch(err) {
    case SL_ALIGNMENT_ERROR:
      app_err = APP_ALIGNMENT_ERROR;
      break;
    case SL_INVALID_ARGUMENT:
      app_err = APP_INVALID_ARGUMENT;
      break;
    case SL_MEMORY_ERROR:
      app_err = APP_MEMORY_ERROR;
      break;
    case SL_OVERFLOW_ERROR:
      app_err = APP_INTERNAL_ERROR;
      break;
    case SL_NO_ERROR:
      app_err = APP_NO_ERROR;
      break;
    default:
      app_err = APP_UNKNOWN_ERROR;
      break;
  }
  return app_err;
}

enum app_error
rdr_to_app_error(enum rdr_error err)
{
  enum app_error app_err = APP_NO_ERROR;
  switch(err) {
    case RDR_DRIVER_ERROR:
      app_err = APP_RENDER_DRIVER_ERROR;
      break;
    case RDR_INTERNAL_ERROR:
      app_err = APP_INTERNAL_ERROR;
      break;
    case RDR_INVALID_ARGUMENT:
      app_err = APP_INVALID_ARGUMENT;
      break;
    case RDR_NO_ERROR:
      app_err = APP_NO_ERROR;
      break;
    case RDR_MEMORY_ERROR:
      app_err = APP_MEMORY_ERROR;
      break;
    case RDR_REF_COUNT_ERROR:
      app_err = APP_INTERNAL_ERROR;
      break;
    default:
      app_err = APP_UNKNOWN_ERROR;
      break;
  }
  return app_err;
}

enum app_error
rsrc_to_app_error(enum rsrc_error err)
{
  enum app_error app_err = APP_NO_ERROR;
  switch(err) {
    case RSRC_INVALID_ARGUMENT:
      app_err = APP_INVALID_ARGUMENT;
      break;
    case RSRC_IO_ERROR:
      app_err = APP_IO_ERROR;
      break;
    case RSRC_MEMORY_ERROR:
      app_err = APP_MEMORY_ERROR;
      break;
    case RSRC_NO_ERROR:
      app_err = APP_NO_ERROR;
      break;
    case RSRC_OVERFOW_ERROR:
      app_err = APP_INTERNAL_ERROR;
      break;
    case RSRC_PARSING_ERROR:
      app_err = APP_INVALID_FILE;
      break;
    default:
      app_err = APP_UNKNOWN_ERROR;
      break;
  }
  return app_err;
}


enum app_error
wm_to_app_error(enum wm_error err)
{
  enum app_error app_err = APP_NO_ERROR;
  switch(err) {
    case WM_INTERNAL_ERROR:
      app_err = APP_INTERNAL_ERROR;
      break;
    case WM_INVALID_ARGUMENT:
      app_err = APP_INTERNAL_ERROR;
      break;
    case WM_MEMORY_ERROR:
      app_err = APP_MEMORY_ERROR;
      break;
    case WM_NO_ERROR:
      app_err = APP_NO_ERROR;
      break;
    default:
      app_err = APP_UNKNOWN_ERROR;
      break;
  }
  return app_err;
}

