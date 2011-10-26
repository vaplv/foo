#ifndef APP_VIEW_C_H
#define APP_VIEW_C_H

struct app_view {
  float transform[16]; /* column major. */
  float ratio;
  float fov_x; /* In radian. */
  float znear, zfar;
};

#endif /* APP_VIEW_C_H */

