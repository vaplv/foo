#ifndef APP_VIEW_H
#define APP_VIEW_H

#include "app/core/app_error.h"

struct app;
struct app_view;

extern enum app_error
app_create_view
  (struct app* app,
   struct app_view** out_view);

extern enum app_error
app_view_ref_get
  (struct app_view* view);

extern enum app_error
app_view_ref_put
  (struct app_view* view);

extern enum app_error
app_look_at
  (struct app_view* view,
   float pos[3],
   float target[3],
   float up[3]);

extern enum app_error
app_perspective
  (struct app_view* view,
   float fov_x, /* In radian. */
   float ratio,
   float znear,
   float zfar);

extern enum app_error
app_view_translate
  (struct app_view* view,
   float x,
   float y,
   float z);

extern enum app_error
app_view_rotate
  (struct app_view* view,
   float pitch, /* X in radian. */
   float yaw,   /* Y in radian. */
   float roll); /* Z in radian. */

extern enum app_error
app_get_view_basis
  (struct app_view* view,
   float pos[3],
   float right[3],
   float up[3],
   float forward[3]);

extern enum app_error
app_get_view_projection
  (struct app_view* view,
   float* fov_x, /* May be NULL. */
   float* ratio, /* May be NULL. */
   float* znear, /* May be NULL. */
   float* zfar); /* May be NULL. */

#endif /* APP_VIEW_H */

