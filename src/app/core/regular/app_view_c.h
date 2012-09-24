#ifndef APP_VIEW_C_H
#define APP_VIEW_C_H

#include "maths/simd/aosf44.h"
#include "sys/ref_count.h"

struct rdr_view;

ALIGN(16) struct app_view {
  struct aosf44 transform;
  struct ref ref;
  struct app* app;
  float ratio;
  float fov_x; /* In radian. */
  float znear, zfar;
};

LOCAL_SYM enum app_error
app_to_rdr_view
  (const struct app* app,
   const struct app_view* view,
   struct rdr_view* render_view);

#endif /* APP_VIEW_C_H */

