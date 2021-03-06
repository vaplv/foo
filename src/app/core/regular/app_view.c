#include "app/core/regular/app_core_c.h"
#include "app/core/regular/app_view_c.h"
#include "app/core/app_view.h"
#include "renderer/rdr_world.h"
#include "sys/math.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "window_manager/wm.h"
#include "window_manager/wm_window.h"
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static FINLINE void 
compute_projection_matrix
  (const float fov_x,
   const float proj_ratio,
   const float znear,
   const float zfar, 
   struct aosf44* proj)
{
  float fov_y = 0.f;
  float f = 0.f;
  float rcp_zrange = 0.f;
  assert(proj);

  fov_y = fov_x / proj_ratio;
  f = cosf(fov_y*0.5f) / sinf(fov_y*0.5f);
  rcp_zrange = 1.f / (znear - zfar);

  proj->c0 = vf4_set(f / proj_ratio, 0.f, 0.f, 0.f);
  proj->c1 = vf4_set(0.f, f, 0.f, 0.f);
  proj->c2 = vf4_set(0.f, 0.f, (zfar + znear)*rcp_zrange, -1.f);
  proj->c3 = vf4_set(0.f, 0.f, (2.f*zfar*znear)*rcp_zrange, 0.f);
}

static void
release_view(struct ref* ref)
{
  struct app_view* view = NULL;
  assert(ref);
  view = CONTAINER_OF(ref, struct app_view, ref);
  MEM_FREE(view->app->allocator, view);
}

/*******************************************************************************
 *
 * View functions.
 *
 ******************************************************************************/
enum app_error
app_create_view(struct app* app, struct app_view** out_view)
{
  struct app_view* view = NULL;
  enum app_error app_err = APP_NO_ERROR;

  if(!app || !out_view) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  view = MEM_ALIGNED_ALLOC(app->allocator, sizeof(struct app_view), 16);
  if(!view) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  view = memset(view, 0, sizeof(struct app_view));
  view->app = app;
  ref_init(&view->ref);

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
    while(!ref_put(&view->ref, release_view));
    view = NULL;
  }
  goto exit;
}

enum app_error
app_view_ref_get(struct app_view* view)
{
  if(!view)
    return APP_INVALID_ARGUMENT;
  ref_get(&view->ref);
  return APP_NO_ERROR;
}

enum app_error
app_view_ref_put(struct app_view* view)
{
  if(!view)
    return APP_INVALID_ARGUMENT;
  ref_put(&view->ref, release_view);
  return APP_NO_ERROR;
}

enum app_error
app_look_at
  (struct app_view* view,
   float pos[3],
   float target[3],
   float up[3])
{
  vf4_t f, s, u;
  vf4_t aos_target;
  vf4_t aos_pos;
  vf4_t aos_up;

  if(!view || !pos || !target || !up)
    return APP_INVALID_ARGUMENT;

  aos_target = vf4_set(target[0], target[1], target[2], 0.f);
  aos_pos = vf4_set(pos[0], pos[1], pos[2], 0.f);
  aos_up = vf4_set(up[0], up[1], up[2], 0.f);

  f = vf4_normalize3(vf4_sub(aos_target, aos_pos));
  u = vf4_normalize3(aos_up);
  s = vf4_cross3(f, u);
  u = vf4_cross3(s, f);

  view->transform.c0 = vf4_xyzd(s, vf4_minus(vf4_dot3(s, aos_pos)));
  view->transform.c1 = vf4_xyzd(u, vf4_minus(vf4_dot3(u, aos_pos)));
  view->transform.c2 = vf4_xyzd(vf4_minus(f), vf4_dot3(f, aos_pos));
  view->transform.c3 = vf4_set(0.f, 0.f, 0.f, 1.f);

  aosf44_transpose(&view->transform, &view->transform);
  return APP_NO_ERROR;
}

enum app_error
app_perspective
  (struct app_view* view,
   float fov_x,
   float ratio,
   float znear,
   float zfar)
{
  if(!view)
    return APP_INVALID_ARGUMENT;
  view->fov_x = fov_x;
  view->ratio = ratio;
  view->znear = znear;
  view->zfar = zfar;
  return APP_NO_ERROR;
}

enum app_error
app_view_translate
  (struct app_view* view,
   float x,
   float y,
   float z)
{
  if(!view)
    return APP_INVALID_ARGUMENT;

  view->transform.c3 = vf4_add(view->transform.c3, vf4_set(x, y, z, 0.f));
  return APP_NO_ERROR;
}

/* XYZ norm. */
enum app_error
app_view_rotate
  (struct app_view* view,
   float pitch,
   float yaw,
   float roll)
{
  struct aosf33 f33;

  if(!view)
    return APP_INVALID_ARGUMENT;
  if(!yaw && !pitch && !roll)
    return APP_NO_ERROR;
  aosf33_rotation(&f33, pitch, yaw, roll);
  aosf44_mulf44
    (&view->transform,
     (struct aosf44[]){{f33.c0, f33.c1, f33.c2, vf4_set(0.f, 0.f, 0.f, 1.f)}},
     &view->transform);
  return APP_NO_ERROR;
}

enum app_error
app_raw_view_transform(struct app_view* view, const struct aosf44* transform)
{
  if(UNLIKELY(!view || !transform))
    return APP_INVALID_ARGUMENT;
  view->transform = *transform;
  return APP_NO_ERROR;
}

enum app_error
app_get_view_basis
  (struct app_view* view,
   float pos[3],
   float right[3],
   float up[3],
   float forward[3])
{
  ALIGN(16) float tmp[4];
  struct aosf44 m;
  vf4_t v;

  if(!view)
    return APP_INVALID_ARGUMENT;

  v = vf4_normalize3(vf4_minus(view->transform.c0));
  vf4_store(tmp, v);
  right[0] = tmp[0];
  right[1] = tmp[1];
  right[2] = tmp[2];

  v = vf4_normalize3(vf4_minus(view->transform.c1));
  vf4_store(tmp, v);
  up[0] = tmp[0];
  up[1] = tmp[1];
  up[2] = tmp[2];

  v = vf4_normalize3(view->transform.c2);
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

enum app_error
app_get_raw_view_transform
  (const struct app_view* view,
   const struct aosf44** transform)
{
  if(!view || !transform)
    return APP_INVALID_ARGUMENT;
  *transform = &view->transform;
  return APP_NO_ERROR;
}

enum app_error
app_get_view_projection
  (struct app_view* view,
   float* fov_x,
   float* ratio,
   float* znear,
   float* zfar)
{
  if(!view)
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

enum app_error
app_get_view_proj_matrix(struct app_view* view, struct aosf44* view_proj)
{
  struct aosf44 proj;

  if(!view || !view_proj)
    return APP_INVALID_ARGUMENT;

  compute_projection_matrix
    (view->fov_x, view->ratio, view->znear, view->zfar, &proj);
  aosf44_mulf44(view_proj, &proj, &view->transform);
  return APP_NO_ERROR;
}

/*******************************************************************************
 *
 * Private functions
 *
 ******************************************************************************/
enum app_error
app_to_rdr_view
  (const struct app* app,
   const struct app_view* view, 
   struct rdr_view* render_view)
{
  struct wm_window_desc win_desc;

  if(UNLIKELY(!app || !view || !render_view))
    return APP_INVALID_ARGUMENT;

  WM(get_window_desc(app->wm.window, &win_desc));

  assert(sizeof(view->transform) == sizeof(render_view->transform));
  aosf44_store(render_view->transform, &view->transform);

  render_view->proj_ratio = view->ratio;
  render_view->fov_x = view->fov_x;
  render_view->znear = view->znear;
  render_view->zfar = view->zfar;
  render_view->x = 0;
  render_view->y = 0;
  render_view->width = win_desc.width;
  render_view->height = win_desc.height;

  return APP_NO_ERROR;
}
