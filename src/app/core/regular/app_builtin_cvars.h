#ifndef APP_BUILTIN_CVARS_H
#define APP_BUILTIN_CVARS_H

#include "app/core/app_error.h"

struct app;

extern enum app_error
app_setup_builtin_cvars
  (struct app* app);

#endif /* APP_BUILTIN_CVARS_H */

