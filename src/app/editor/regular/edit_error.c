#include "app/editor/regular/edit_error_c.h"

enum edit_error
app_to_edit_error(enum app_error err)
{
  enum edit_error edit_err = EDIT_NO_ERROR;
  switch(err) {
    case APP_INVALID_ARGUMENT:
      edit_err = EDIT_INVALID_ARGUMENT;
      break;
    case APP_MEMORY_ERROR:
      edit_err = EDIT_MEMORY_ERROR;
      break;
    case APP_NO_ERROR:
      edit_err = EDIT_NO_ERROR;
      break;
    default:
      edit_err = EDIT_UNKNOWN_ERROR;
      break;
  }
  return edit_err;
}

enum edit_error
sl_to_edit_error(enum sl_error err)
{
  enum edit_error edit_err = EDIT_NO_ERROR;
  switch(err) {
    case SL_INVALID_ARGUMENT:
      edit_err = EDIT_INVALID_ARGUMENT;
      break;
    case SL_MEMORY_ERROR:
      edit_err = EDIT_MEMORY_ERROR;
      break;
    case SL_NO_ERROR:
      edit_err = EDIT_NO_ERROR;
      break;
    default:
      edit_err = EDIT_UNKNOWN_ERROR;
      break;
  }
  return edit_err;
}

enum edit_error
wm_to_edit_error(enum wm_error err)
{
  enum edit_error edit_err = EDIT_NO_ERROR;
  switch(err) {
    case WM_INVALID_ARGUMENT:
      edit_err = EDIT_INVALID_ARGUMENT;
      break;
    case WM_MEMORY_ERROR:
      edit_err = EDIT_MEMORY_ERROR;
      break;
    case WM_NO_ERROR:
      edit_err = EDIT_NO_ERROR;
      break;
    default:
      edit_err = EDIT_UNKNOWN_ERROR;
      break;
  }
  return edit_err;
}

