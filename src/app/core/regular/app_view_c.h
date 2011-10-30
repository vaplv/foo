#ifndef APP_VIEW_C_H
#define APP_VIEW_C_H

#include "maths/simd/aosf44.h"

struct app_view {
  struct aosf44 transform;
  float ratio;
  float fov_x; /* In radian. */
  float znear, zfar;
};

#endif /* APP_VIEW_C_H */

