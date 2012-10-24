#include "app/core/regular/app_core_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_view_c.h"
#include "app/core/app_imdraw.h"
#include "renderer/rdr_world.h"
#include "renderer/rdr_frame.h"
#include "renderer/rdr_imdraw.h"
#include "sys/sys.h"
#include "window_manager/wm.h"
#include "window_manager/wm_window.h"
#include <assert.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static int
app_to_rdr_imdraw_flag(int app_flag)
{
  int flag = RDR_IMDRAW_FLAG_NONE;

  #define SETUP_FLAG(f) \
    flag |= (-((app_flag & CONCAT(APP_, f)) != 0)) & CONCAT(RDR_, f)
  SETUP_FLAG(IMDRAW_FLAG_UPPERMOST_LAYER);
  SETUP_FLAG(IMDRAW_FLAG_FIXED_SCREEN_SIZE);
  #undef SETUP_FLAG
  return flag;
}

static enum rdr_im_vector_marker
app_to_rdr_im_vector_marker(enum app_im_vector_marker marker)
{
  enum rdr_im_vector_marker rmarker = RDR_IM_VECTOR_MARKER_NONE;

  switch(marker) {
    case APP_IM_VECTOR_CUBE_MARKER:
      rmarker = RDR_IM_VECTOR_CUBE_MARKER;
      break;
    case APP_IM_VECTOR_CONE_MARKER:
      rmarker = RDR_IM_VECTOR_CONE_MARKER;
      break;
    case APP_IM_VECTOR_MARKER_NONE:
      rmarker = RDR_IM_VECTOR_MARKER_NONE;
      break;
    default:
      assert(0);
      break;
  };
  return rmarker;
}

static FINLINE void
setup_render_view(struct app* app, struct rdr_view* render_view)
{
  struct wm_window_desc win_desc;
  memset(&win_desc, 0, sizeof(win_desc));
  assert(app && render_view);

  assert(sizeof(app->view->transform) == sizeof(render_view->transform));
  WM(get_window_desc(app->wm.window, &win_desc));

  aosf44_store(render_view->transform, &app->view->transform);
  render_view->proj_ratio = app->view->ratio;
  render_view->fov_x = app->view->fov_x;
  render_view->znear = app->view->znear;
  render_view->zfar = app->view->zfar;
  render_view->x = 0;
  render_view->y = 0;
  render_view->width = win_desc.width;
  render_view->height = win_desc.height;
}

/*******************************************************************************
 *
 * Im draw functions.
 *
 ******************************************************************************/
enum app_error
app_imdraw_parallelepiped
  (struct app* app,
   const int flag,
   const uint32_t in_pick_id,
   const float pos[3],
   const float size[3],
   const float rotation[3],
   const float solid_color[4],
   const float wire_color[4])
{
  struct rdr_view render_view;
  const uint32_t pick_id = in_pick_id >= APP_PICK_ID_MAX 
    ? APP_PICK_NONE 
    : APP_PICK(in_pick_id, APP_PICK_GROUP_IMDRAW);
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  memset(&render_view, 0, sizeof(render_view));

  if(UNLIKELY(!app || !pos || !size || !rotation)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  setup_render_view(app, &render_view);
  rdr_err = rdr_frame_imdraw_parallelepiped
    (app->rdr.frame,
     &render_view,
     app_to_rdr_imdraw_flag(flag),
     pick_id,
     pos,
     size,
     rotation,
     solid_color,
     wire_color);
  if(rdr_err != RDR_NO_ERROR) {
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }
exit:
  return app_err;
error:
  goto exit;
}

enum app_error
app_imdraw_ellipse
  (struct app* app,
   const int flag,
   const uint32_t in_pick_id,
   const float pos[3],
   const float size[2],
   const float rotation[3],
   const float color[4])
{
  struct rdr_view render_view;
  const uint32_t pick_id = in_pick_id >= APP_PICK_ID_MAX 
    ? APP_PICK_NONE 
    : APP_PICK(in_pick_id, APP_PICK_GROUP_IMDRAW);
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  memset(&render_view, 0, sizeof(render_view));

  if(UNLIKELY(!app || !pos || !size || !rotation)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  setup_render_view(app, &render_view);
  rdr_err = rdr_frame_imdraw_ellipse
    (app->rdr.frame,
     &render_view,
     app_to_rdr_imdraw_flag(flag),
     pick_id,
     pos,
     size,
     rotation,
     color);
  if(rdr_err != RDR_NO_ERROR) {
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }
exit:
  return app_err;
error:
  goto exit;
}

enum app_error
app_imdraw_grid
  (struct app* app,
   const int flag,
   const uint32_t in_pick_id,
   const float pos[3],
   const float size[2],
   const float rotation[3],
   const unsigned int ndiv[2],
   const unsigned int nsubdiv[2],
   const float color[3],
   const float subcolor[3],
   const float vaxis_color[3],
   const float haxis_color[3])
{
  struct rdr_view render_view;
  const uint32_t pick_id = in_pick_id >= APP_PICK_ID_MAX 
    ? APP_PICK_NONE 
    : APP_PICK(in_pick_id, APP_PICK_GROUP_IMDRAW);
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  memset(&render_view, 0, sizeof(render_view));

  if(UNLIKELY
  (  !app
  || !pos
  || !size
  || !rotation
  || !ndiv
  || !nsubdiv
  || !color
  || !subcolor
  || !vaxis_color
  || !haxis_color)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  setup_render_view(app, &render_view);
  rdr_err = rdr_frame_imdraw_grid
    (app->rdr.frame,
     &render_view,
     app_to_rdr_imdraw_flag(flag),
     pick_id,
     pos,
     size,
     rotation,
     ndiv,
     nsubdiv,
     color,
     subcolor,
     vaxis_color,
     haxis_color);
  if(rdr_err != RDR_NO_ERROR) {
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }
exit:
  return app_err;
error:
  goto exit;
}

enum app_error
app_imdraw_vector
  (struct app* app,
   const int flag,
   const uint32_t in_pick_id,
   const enum app_im_vector_marker start_marker,
   const enum app_im_vector_marker end_marker,
   const float start[3],
   const float end[3],
   const float color[3])
{
  struct rdr_view render_view;
  const uint32_t pick_id = in_pick_id >= APP_PICK_ID_MAX 
    ? APP_PICK_NONE 
    : APP_PICK(in_pick_id, APP_PICK_GROUP_IMDRAW);
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  memset(&render_view, 0, sizeof(render_view));

  if(UNLIKELY(!app || !start || !end || !color)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  setup_render_view(app, &render_view);
  rdr_err = rdr_frame_imdraw_vector
    (app->rdr.frame,
     &render_view,
     app_to_rdr_imdraw_flag(flag),
     pick_id,
     app_to_rdr_im_vector_marker(start_marker),
     app_to_rdr_im_vector_marker(end_marker),
     start,
     end,
     color);
  if(rdr_err != RDR_NO_ERROR) {
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }
exit:
  return app_err;
error:
  goto exit;
}

enum app_error
app_imdraw_pick
  (struct app* app,
   const unsigned int pos[2],
   const unsigned int size[2])
{
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(!app || !pos || !size)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  rdr_err = rdr_frame_pick_imdraw(app->rdr.frame, pos, size);
  if(rdr_err != RDR_NO_ERROR) {
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }
exit:
  return app_err;
error:
  goto exit;
}

