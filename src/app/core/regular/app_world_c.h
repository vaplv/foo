#ifndef APP_WORLD_C_H
#define APP_WORLD_C_H

#include "app/core/app_error.h"

struct app;
struct app_view;
struct app_world;

extern enum app_error
app_draw_world
  (struct app_world* world,
   const struct app_view* view);

#endif /* APP_WORLD_C_H */
