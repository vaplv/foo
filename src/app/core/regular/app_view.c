#include "app/core/regular/app_view_c.h"
#include "app/core/app_view.h"
#include "sys/sys.h"
#include <math.h>
#include <stdlib.h>

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

  aosf44_identity(&view->transform);
  view->ratio = 4.f / 3.f;
  view->fov_x = DEG2RAD(90.f);
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
  vf4_t f, s, u;
  vf4_t aos_target;
  vf4_t aos_pos;
  vf4_t aos_up;

  if(!app || !view || !pos || !target || !up)
    return APP_INVALID_ARGUMENT;

  aos_target = vf4_set(target[0], target[1], target[3], 0.f);
  aos_pos = vf4_set(pos[0], pos[1], pos[2], 0.f);
  aos_up = vf4_set(up[0], up[1], up[2], 0.f);

  f = vf4_normalize(vf4_sub(aos_target, aos_pos));
  u = vf4_normalize(aos_up);
  s = vf4_cross3(f, u);
  u = vf4_cross3(s, f);

  view->transform.c0 = vf4_xyzd(s, vf4_minus(vf4_dot3(s, aos_pos)));
  view->transform.c1 = vf4_xyzd(u, vf4_minus(vf4_dot3(u, aos_pos)));
  view->transform.c2 = vf4_xyzd(vf4_minus(f), vf4_dot3(f, aos_pos));
  view->transform.c3 = vf4_set(0.f, 0.f, 0.f, 1.f);

  aosf44_transpose(&view->transform, &view->transform);
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

  view->transform.c3 = vf4_add(view->transform.c3, vf4_set(x, y, z, 0.f));
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
  struct aosf44 f44;
  float c1, c2, c3;
  float s1, s2, s3;

  if(!app || !view)
    return APP_INVALID_ARGUMENT;

  if(!yaw && !pitch && !roll)
    return APP_NO_ERROR;

  c1 = cos(pitch);
  c2 = cos(yaw);
  c3 = cos(roll);
  s1 = sin(pitch);
  s2 = sin(yaw);
  s3 = sin(roll);

  f44.c0 = vf4_set(c2*c3, c1*s3 + c3*s1*s2, s1*s3 - c1*c3*s2, 0.f);
  f44.c1 = vf4_set(-c2*s3, c1*c3 - s1*s2*s3, c1*s2*s3 + c3*s1, 0.f);
  f44.c2 = vf4_set(s2, -c2*s1, c1*c2, 0.f);
  f44.c3 = vf4_set(0.f, 0.f, 0.f, 1.f);
  aosf44_mulf44(&view->transform, &f44, &view->transform);

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
  ALIGN(16) float tmp[4];
  struct aosf44 m;
  vf4_t v;

  if(!app || !view)
    return APP_INVALID_ARGUMENT;

  v = vf4_normalize(vf4_minus(aosf44_row0(&view->transform)));
  vf4_store(tmp, v);
  right[0] = tmp[0];
  right[1] = tmp[1];
  right[2] = tmp[2];

  v = vf4_normalize(vf4_minus(aosf44_row1(&view->transform)));
  vf4_store(tmp, v);
  up[0] = tmp[0];
  up[1] = tmp[1];
  up[2] = tmp[2];

  v = vf4_normalize(aosf44_row1(&view->transform));
  vf4_store(tmp, v);
  forward[0] = tmp[0];
  forward[1] = tmp[1];
  forward[2] = tmp[2];

  v = aosf44_inverse(&m, &view->transform);
  v = vf4_sel(view->transform.c3, vf4_zero(), vf4_eq(v, vf4_zero()));
  vf4_store(tmp, v);
  pos[0] = tmp[0];
  pos[1] = tmp[1];
  pos[2] = tmp[2];

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


