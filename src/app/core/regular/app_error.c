#include "app/core/regular/app_error_c.h"

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

