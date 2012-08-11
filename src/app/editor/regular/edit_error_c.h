#ifndef EDIT_ERROR_C_H
#define EDIT_ERROR_C_H

#include "app/core/app_error.h"
#include "app/editor/edit_error.h"
#include "window_manager/wm_error.h"
#include "stdlib/sl_error.h"

extern enum edit_error
app_to_edit_error
  (enum app_error err);

extern enum edit_error
sl_to_edit_error
  (enum sl_error err);

extern enum edit_error
wm_to_edit_error
  (enum wm_error err);

#endif /* EDIT_ERROR_C_H */

