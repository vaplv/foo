/* TODO use the math library when it will be available. */
#include "app/core/regular/app_view_c.h"
#include "app/core/app_view.h"
#include "sys/sys.h"
#include <math.h>
#include <stdlib.h>

/* TODO remove me */
#include <assert.h>
#include <stdbool.h>

#define DEG2RAD(x) ((x)*0.0174532925199432957692369076848861L)

EXPORT_SYM enum app_error
app_create_view(struct app* app, struct app_view** out_view)
{
  struct app_view* view = NULL;
  enum app_error app_err = APP_NO_ERROR;

  if(!app || !out_view) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  view = calloc(1, sizeof(struct app_view));
  if(!view) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }

  view->transform[0] = 1.f;
  view->transform[5] = 1.f;
  view->transform[10] = 1.f;
  view->transform[15] = 1.f;
  view->ratio = 4.f / 3.f;
  view->fov_x = DEG2RAD(80.f);
  view->znear = 1.f;
  view->zfar = 1000.f;

exit:
  if(out_view)
    *out_view = view;
  return app_err;

error:
  if(view) {
    free(view);
    view = NULL;
  }
  goto exit;
}

EXPORT_SYM enum app_error
app_free_view(struct app* app, struct app_view* view)
{
  if(!app || !view)
    return APP_INVALID_ARGUMENT;
  free(view);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_look_at
  (struct app* app,
   struct app_view* view,
   float pos[3],
   float target[3],
   float up[3])
{
  float f[3] = {0.f, 0.f, 0.f};
  float s[3] = {0.f, 0.f, 0.f};
  float u[3] = {0.f, 0.f, 0.f};
  float rcp_len = 0.f;

  if(!app || !view || !pos || !target || !up)
    return APP_INVALID_ARGUMENT;

  f[0] = target[0] - pos[0];
  f[1] = target[1] - pos[1];
  f[2] = target[2] - pos[2];
  rcp_len = 1.f / sqrtf(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
  f[0] *= rcp_len;
  f[1] *= rcp_len;
  f[2] *= rcp_len;

  rcp_len = 1.f / sqrtf(up[0]*up[0] + up[1]*up[1] + up[2]*up[2]);
  u[0] = up[0] * rcp_len;
  u[1] = up[1] * rcp_len;
  u[2] = up[2] * rcp_len;

  s[0] = f[1]*u[2] - f[2]*u[1];
  s[1] = f[2]*u[0] - f[0]*u[2];
  s[2] = f[0]*u[1] - f[1]*u[0];

  u[0] = s[1]*f[2] - s[2]*f[1];
  u[1] = s[2]*f[0] - s[0]*f[2];
  u[2] = s[0]*f[1] - s[1]*f[0];

  view->transform[0] = s[0];
  view->transform[1] = u[0];
  view->transform[2] = -f[0];
  view->transform[3] = 0.f;

  view->transform[4] = s[1];
  view->transform[5] = u[1];
  view->transform[6] = -f[1];
  view->transform[7] = 0.f;

  view->transform[8] = s[2];
  view->transform[9] = u[2];
  view->transform[10] = -f[2];
  view->transform[11] = 0.f;

  view->transform[12] = -(s[0]*pos[0] + s[1]*pos[1] + s[2]*pos[2]);
  view->transform[13] = -(u[0]*pos[0] + u[1]*pos[1] + u[2]*pos[2]);
  view->transform[14] =  (f[0]*pos[0] + f[1]*pos[1] + f[2]*pos[2]);
  view->transform[15] = 1.f;

  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_perspective
  (struct app* app,
   struct app_view* view,
   float fov_x,
   float ratio,
   float znear,
   float zfar)
{
  if(!app || !view)
    return APP_INVALID_ARGUMENT;
  view->fov_x = fov_x;
  view->ratio = ratio;
  view->znear = znear;
  view->zfar = zfar;
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_view_translate
  (struct app* app,
   struct app_view* view,
   float x,
   float y,
   float z)
{
  if(!app || !view)
    return APP_INVALID_ARGUMENT;

  view->transform[12] =
      view->transform[0] * x
    + view->transform[4] * y
    + view->transform[8] * z
    + view->transform[12];
  view->transform[13] =
      view->transform[1] * x
    + view->transform[5] * y
    + view->transform[9] * z
    + view->transform[13];
  view->transform[14] =
      view->transform[2] * x
    + view->transform[6] * y
    + view->transform[10] * z
    + view->transform[14];
  view->transform[15] =
      view->transform[3] * x
    + view->transform[7] * y
    + view->transform[11] * z
    + view->transform[15];

  return APP_NO_ERROR;
}

/* XYZ norm. */
EXPORT_SYM enum app_error
app_view_rotate
  (struct app* app,
   struct app_view* view,
   float pitch,
   float yaw,
   float roll)
{
  float m[9];
  float transform[16];
  float c1, c2, c3;
  float s1, s2, s3;

  if(!app || !view)
    return APP_INVALID_ARGUMENT;

  c1 = cosf(pitch);
  c2 = cosf(yaw);
  c3 = cosf(roll);

  s1 = cosf(pitch);
  s2 = cosf(yaw);
  s3 = cosf(roll);

  m[0] = c2*c3;
  m[1] = c1*s3 + c3*s1*s2;
  m[2] = s1*s3 - c1*c3*s2;

  m[3] = -c2*s3;
  m[4] = c1*c3 - s1*s2*s3;
  m[5] = c1*s2*s3 + c3*s1;

  m[6] = s2;
  m[7] = -c2*s1;
  m[8] = c1*c2;

  transform[0] =
      view->transform[0]*m[0]
    + view->transform[4]*m[1]
    + view->transform[8]*m[2];
  transform[4] =
      view->transform[0]*m[3]
    + view->transform[4]*m[4]
    + view->transform[8]*m[5];
  transform[8] =
      view->transform[0]*m[6]
    + view->transform[4]*m[7]
    + view->transform[8]*m[8];

  transform[1] =
      view->transform[1]*m[0]
    + view->transform[5]*m[1]
    + view->transform[9]*m[2];
  transform[5] =
      view->transform[1]*m[3]
    + view->transform[5]*m[4]
    + view->transform[9]*m[5];
  transform[9] =
      view->transform[1]*m[6]
    + view->transform[5]*m[7]
    + view->transform[9]*m[8];

  transform[2] =
      view->transform[2]*m[0]
    + view->transform[6]*m[1]
    + view->transform[10]*m[2];
  transform[6] =
      view->transform[2]*m[3]
    + view->transform[6]*m[4]
    + view->transform[10]*m[5];
  transform[10] =
      view->transform[2]*m[6]
    + view->transform[6]*m[7]
    + view->transform[10]*m[8];

  transform[3] = view->transform[3];
  transform[7] = view->transform[7];
  transform[11] = view->transform[11];
  transform[12] = view->transform[12];
  transform[13] = view->transform[13];
  transform[14] = view->transform[14];
  transform[15] = view->transform[15];

  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_get_view_space
  (struct app* app,
   struct app_view* view,
   float pos[3] UNUSED,
   float right[3] UNUSED,
   float up[3] UNUSED,
   float forward[3] UNUSED)
{
  if(!app || !view)
    return APP_INVALID_ARGUMENT;

  /* TODO */
  assert(false);
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_get_view_projection
  (struct app* app,
   struct app_view* view,
   float* fov_x,
   float* ratio,
   float* znear,
   float* zfar)
{
  if(!app || !view)
    return APP_INVALID_ARGUMENT;

  if(fov_x)
    *fov_x = view->fov_x;
  if(ratio)
    *ratio = view->ratio;
  if(znear)
    *znear = view->znear;
  if(zfar)
    *zfar = view->zfar;

  return APP_NO_ERROR;
}


