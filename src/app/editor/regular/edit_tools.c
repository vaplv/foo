#include "app/core/app_imdraw.h"
#include "app/core/app_pick_id.h"
#include "app/editor/edit.h"
#include "app/editor/edit_model_instance_selection.h"
#include "app/editor/regular/edit_imgui.h"
#include "app/editor/regular/edit_tools.h"
#include "sys/math.h"
#include <assert.h>

/*******************************************************************************
 *
 * Helper functions
 *
 ******************************************************************************/
static void
draw_basis_circles
  (struct app* app,
   const float pos[3],
   const float size,
   const float col_x[3],
   const float col_y[3],
   const float col_z[3],
   const uint32_t pick_id_x,
   const uint32_t pick_id_y,
   const uint32_t pick_id_z)
{
  assert(app && pos);

  #define DRAW_CIRCLE(pitch, yaw, roll, color, pick) \
    APP(imdraw_ellipse \
      (app, \
       APP_IMDRAW_FLAG_UPPERMOST_LAYER | APP_IMDRAW_FLAG_FIXED_SCREEN_SIZE, \
       pick, \
       pos, \
       (float[]){size, size}, \
       (float[]){(pitch), (yaw), (roll)}, \
       (color)))

  DRAW_CIRCLE(PI * 0.5f, 0.f, 0.f, col_x, pick_id_x);
  DRAW_CIRCLE(0.f, 0.f, 0.f, col_y, pick_id_y);
  DRAW_CIRCLE(0.f, PI * 0.5f, 0.f, col_z, pick_id_z);

  #undef DRAW_CIRCLE
}

/*******************************************************************************
 *
 * Edit tool functions
 *
 ******************************************************************************/
enum edit_error
edit_scale_tool
  (struct edit_imgui* imgui,
   struct edit_model_instance_selection* selection,
   const float sensitivity)
{
  const float size = 0.2f;
  float pivot_pos[3] = {0.f, 0.f, 0.f};
  float val[3] = { 1.f, 1.f, 1.f };

  if(UNLIKELY(!imgui || !selection))
    return EDIT_INVALID_ARGUMENT;

  EDIT(get_model_instance_selection_pivot(selection, pivot_pos));

  EDIT(imgui_scale_tool
    (imgui, EDIT_IMGUI_ID, sensitivity, pivot_pos, size,
     (float[]){1.f, 0.f, 0.f},
     (float[]){0.f, 1.f, 0.f},
     (float[]){0.f, 0.f, 1.f},
     val));

  if(val[0] == 1.f && val[1] == 1.f && val[2] == 1.f)
    return EDIT_NO_ERROR;

  EDIT(scale_model_instance_selection(selection, true, val));
  return EDIT_NO_ERROR;
}

enum edit_error
edit_translate_tool
  (struct edit_imgui* imgui,
   struct edit_model_instance_selection* selection,
   const float sensitivity)
{
  const float size = 0.2f;
  float pivot_pos[3] = {0.f, 0.f, 0.f};
  float val[3] = { 0, 0, 0 };

  if(UNLIKELY(!imgui || !selection))
    return EDIT_INVALID_ARGUMENT;

  EDIT(get_model_instance_selection_pivot(selection, pivot_pos));

  EDIT(imgui_translate_tool
    (imgui, EDIT_IMGUI_ID, sensitivity, pivot_pos, size,
     (float[]){1.f, 0.f, 0.f},
     (float[]){0.f, 1.f, 0.f},
     (float[]){0.f, 0.f, 1.f},
     val));

  if(!val[0] && !val[1] && !val[2])
    return EDIT_NO_ERROR;

  EDIT(translate_model_instance_selection(selection, val));
  return EDIT_NO_ERROR;
}

enum edit_error
edit_rotate_tool
  (struct edit_imgui* imgui,
   struct edit_model_instance_selection* selection,
   const float sensitivity)
{
  const float size = 0.2f;
  float pivot_pos[3] = {0.f, 0.f, 0.f};
  float val[3] = { 0, 0, 0 };

  if(UNLIKELY(!imgui || !selection))
    return EDIT_INVALID_ARGUMENT;

  EDIT(get_model_instance_selection_pivot(selection, pivot_pos));

  EDIT(imgui_rotate_tool
    (imgui, EDIT_IMGUI_ID, sensitivity, pivot_pos, size,
     (float[]){1.f, 0.f, 0.f},
     (float[]){0.f, 1.f, 0.f},
     (float[]){0.f, 0.f, 1.f},
     val));

  if(!val[0] && !val[1] && !val[2])
    return EDIT_NO_ERROR;

  EDIT(rotate_model_instance_selection(selection, true, val));
  return EDIT_NO_ERROR;
}

enum edit_error
edit_draw_pivot
  (struct app* app,
   const float pos[3],
   const float size,
   const float color[3])
{
  draw_basis_circles
    (app,
     pos,
     size,
     color,
     color,
     color,
     APP_PICK_ID_MAX,
     APP_PICK_ID_MAX,
     APP_PICK_ID_MAX);
  return EDIT_NO_ERROR;
}
