#ifndef APP_TERM_H
#define APP_TERM_H

#include "app/core/app_error.h"
#include <stdbool.h>

struct app;

extern enum app_error
app_init_term
  (struct app* app);

extern enum app_error
app_shutdown_term
  (struct app* app);

extern enum app_error
app_setup_term
  (struct app* app,
   bool enable);

#endif /* APP_TERM_H */

