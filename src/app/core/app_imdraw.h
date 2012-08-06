#ifndef APP_IMDRAW_H
#define APP_IMDRAW_H

#include "app/core/app_error.h"

struct app;
struct app_view;

enum app_imdraw_flag {
  APP_IMDRAW_FLAG_UPPERMOST_LAYER = BIT(0),
  APP_IMDRAW_FLAG_FIXED_SCREEN_SIZE = BIT(1),
  APP_IMDRAW_FLAG_NONE = 0
};

enum app_im_vector_marker {
  APP_IM_VECTOR_CUBE_MARKER,
  APP_IM_VECTOR_CONE_MARKER,
  APP_IM_VECTOR_MARKER_NONE
};

extern enum app_error
app_imdraw_parallelepiped
  (struct app* app,
   int flag,
   const float pos[3],
   const float size[3],
   const float rotation[3],
   const float solid_color[4], /* May be NULL <=> No solid parallelepiped. */
   const float wire_color[4]); /* May be NULL <=> No wire parallelepiped. */

extern enum app_error
app_imdraw_ellipse
  (struct app* app,
   int flag,
   const float pos[3],
   const float size[2],
   const float rotation[3],
   const float color[4]);

extern enum app_error
app_imdraw_grid
  (struct app* app,
   int flag,
   const float pos[3],
   const float size[2],
   const float rotation[3],
   const unsigned int ndiv[2],
   const unsigned int nsubdiv[2],
   const float color[3],
   const float subcolor[3],
   const float vaxis_color[3],
   const float haxis_color[3]);

extern enum app_error
app_imdraw_vector
  (struct app* app,
   int flag,
   enum app_im_vector_marker start_marker,
   enum app_im_vector_marker end_marker,
   const float start[3],
   const float end[3],
   const float color[4]);

#endif /* APP_IMDRAW_H */

