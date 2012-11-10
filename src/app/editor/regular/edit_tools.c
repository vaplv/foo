#include "app/core/app_imdraw.h"
#include "app/core/app_pick_id.h"
#include "app/editor/regular/edit_tools.h"
#include "sys/math.h"

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

static void
draw_basis
  (struct app* app,
   const float pos[3],
   const float size,
   const enum app_im_vector_marker end_marker,
   const float col_x[3],
   const float col_y[3],
   const float col_z[3],
   const uint32_t pick_origin,
   const uint32_t pick_id_x,
   const uint32_t pick_id_y,
   const uint32_t pick_id_z)
{
  assert(app && pos);

  #define DRAW_VECTOR(end, color, pick)  \
   APP(imdraw_vector \
      (app, \
       APP_IMDRAW_FLAG_FIXED_SCREEN_SIZE | APP_IMDRAW_FLAG_UPPERMOST_LAYER, \
       pick, \
       APP_IM_VECTOR_MARKER_NONE, \
       end_marker, \
       pos, \
       end, \
       color))

  DRAW_VECTOR
    (((float[]){pos[0] + size, pos[1], pos[2]}), col_x, pick_id_x);
  DRAW_VECTOR
    (((float[]){pos[0], pos[1] + size, pos[2]}), col_y, pick_id_y);
  DRAW_VECTOR
    (((float[]){pos[0], pos[1], pos[2] + size}), col_z, pick_id_z);

  #undef DRAW_VECTOR

  APP(imdraw_parallelepiped
    (app,
     APP_IMDRAW_FLAG_FIXED_SCREEN_SIZE | APP_IMDRAW_FLAG_UPPERMOST_LAYER,
     pick_origin,
     pos,
     (float[]){0.05f, 0.05f, 0.05f},
     (float[]){0.f, 0.f, 0.f},
     (float[]){1.f, 1.f, 0.f, 0.5f},
     (float[]){1.f, 1.f, 0.f, 1.f}));
}


/*******************************************************************************
 *
 * Edit tool functions
 *
 ******************************************************************************/
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

enum edit_error
edit_draw_rotate_tool
  (struct app* app,
   const float pos[3],
   const float size,
   const float color_x[3],
   const float color_y[3],
   const float color_z[3])
{
  draw_basis_circles
    (app,
     pos,
     size,
     color_x,
     color_y,
     color_z,
     EDIT_PICK_ID_ROTATE_X,
     EDIT_PICK_ID_ROTATE_Y,
     EDIT_PICK_ID_ROTATE_Z);
  return EDIT_NO_ERROR;
}

enum edit_error
edit_draw_scale_tool
  (struct app* app,
   const float pos[3],
   const float size,
   const float color_x[3],
   const float color_y[3],
   const float color_z[3])
{
  draw_basis
    (app,
     pos,
     size,
     APP_IM_VECTOR_CUBE_MARKER,
     color_x,
     color_y,
     color_z,
     EDIT_PICK_ID_SCALE,
     EDIT_PICK_ID_SCALE_X,
     EDIT_PICK_ID_SCALE_Y,
     EDIT_PICK_ID_SCALE_Z);
  return EDIT_NO_ERROR;
}

enum edit_error
edit_draw_translate_tool
   (struct app* app,
   const float pos[3],
   const float size,
   const float color_x[3],
   const float color_y[3],
   const float color_z[3])
{
  draw_basis
    (app,
     pos,
     size,
     APP_IM_VECTOR_CONE_MARKER,
     color_x,
     color_y,
     color_z,
     EDIT_PICK_ID_TRANSLATE,
     EDIT_PICK_ID_TRANSLATE_X,
     EDIT_PICK_ID_TRANSLATE_Y,
     EDIT_PICK_ID_TRANSLATE_Z);
  return EDIT_NO_ERROR;
}

