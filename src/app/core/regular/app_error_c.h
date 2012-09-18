#ifndef APP_ERROR_C_H
#define APP_ERROR_C_H

#include "app/core/app_error.h"
#include "renderer/rdr_error.h"
#include "resources/rsrc_error.h"
#include "stdlib/sl_error.h"
#include "sys/sys.h"
#include "window_manager/wm_error.h"

LOCAL_SYM enum app_error 
sl_to_app_error
  (enum sl_error err);

LOCAL_SYM enum app_error
rdr_to_app_error
  (enum rdr_error err);

LOCAL_SYM enum app_error
rsrc_to_app_error
  (enum rsrc_error err);

LOCAL_SYM enum app_error
wm_to_app_error
  (enum wm_error err);

#endif /* APP_ERROR_C_H */

