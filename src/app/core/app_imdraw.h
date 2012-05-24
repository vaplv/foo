#ifndef APP_IMDRAW_H
#define APP_IMDRAW_H

#include "app/core/app_error.h"

struct app;
struct app_view;

extern enum app_error
app_imdraw_parallelepiped
  (struct app* app,
   const float pos[3],
   const float size[3],
   const float rotation[3],
   const float solid_color[4], /* May be NULL <=> No solid parallelepiped. */
   const float wire_color[4]); /* May be NULL <=> No wire parallelepiped. */

extern enum app_error
app_imdraw_ellipse
  (struct app* app,
   const float pos[3],
   const float size[2],
   const float rotation[3],
   const float color[4]);

#endif /* APP_IMDRAW_H */

