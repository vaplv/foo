#include "app/core/app_error.h"
#include "app/editor/gtk/edit_gtk_c.h"
#include "app/editor/edit_error.h"
#include "sys/sys.h"

EXPORT_SYM enum edit_error
app_to_edit_error(enum app_error err)
{
  enum edit_error edit_err = EDIT_NO_ERROR;
  switch(err) {
    case APP_INVALID_ARGUMENT:
      edit_err = EDIT_INVALID_ARGUMENT;
      break;
    case APP_IO_ERROR:
      edit_err = EDIT_IO_ERROR;
      break;
    case APP_MEMORY_ERROR:
      edit_err = EDIT_MEMORY_ERROR;
      break;
    case APP_NO_ERROR:
      edit_err = EDIT_NO_ERROR;
      break;
    default:
      edit_err = EDIT_UNKOWN_ERROR;
      break;
  }
  return edit_err;
}

