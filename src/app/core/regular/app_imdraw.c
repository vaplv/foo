#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_view_c.h"
#include "app/core/app_imdraw.h"
#include "renderer/rdr_world.h"
#include "renderer/rdr_frame.h"
#include "sys/sys.h"
#include "window_manager/wm.h"
#include "window_manager/wm_window.h"
#include <assert.h>
#include <string.h>

EXPORT_SYM enum app_error
app_imdraw_parallelepiped
  (struct app* app,
   const float pos[3],
   const float size[3],
   const float rotation[3],
   const float solid_color[4],
   const float wire_color[4])
{
  struct rdr_view render_view;
  struct wm_window_desc win_desc;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  memset(&render_view, 0, sizeof(render_view));
  memset(&win_desc, 0, sizeof(win_desc));

  if(UNLIKELY(app || !pos || !size || !rotation)) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  assert(sizeof(app->view->transform) == sizeof(render_view.transform));
  WM(get_window_desc(app->wm.window, &win_desc));

  aosf44_store(render_view.transform, &app->view->transform);
  render_view.proj_ratio = app->view->ratio;
  render_view.fov_x = app->view->fov_x;
  render_view.znear = app->view->znear;
  render_view.zfar = app->view->zfar;
  render_view.x = 0;
  render_view.y = 0;
  render_view.width = win_desc.width;
  render_view.height = win_desc.height;

  rdr_err = rdr_frame_imdraw_parallelepiped
    (app->rdr.frame,
     &render_view,
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

