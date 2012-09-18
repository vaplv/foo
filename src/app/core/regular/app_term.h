#ifndef APP_TERM_H
#define APP_TERM_H

#include "app/core/app_error.h"
#include "sys/sys.h"
#include <stdbool.h>

struct app;

LOCAL_SYM enum app_error
app_init_term
  (struct app* app);

LOCAL_SYM enum app_error
app_shutdown_term
  (struct app* app);

LOCAL_SYM enum app_error
app_setup_term
  (struct app* app,
   bool enable);

#endif /* APP_TERM_H */

