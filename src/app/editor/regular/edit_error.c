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
    default:
      edit_err = EDIT_UNKNOWN_ERROR;
      break;
  }
  return edit_err;
}

