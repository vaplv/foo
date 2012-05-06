#ifndef APP_CVAR_C_H
#define APP_CVAR_C_H

#include "app/core/app_error.h"

struct app;

extern enum app_error
app_init_cvar_system
  (struct app* app);

extern enum app_error
app_shutdown_cvar_system
  (struct app* app);

#endif /* APP_CVAR_C_H */

