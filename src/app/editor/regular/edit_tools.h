#ifndef EDIT_TOOLS_H
#define EDIT_TOOLS_H

#include "app/editor/edit_error.h"
#include "sys/sys.h"

struct app;
struct edit_imgui;
struct edit_model_instance_selection;

enum
{
  EDIT_PICK_ID_ROTATE = BIT(0),
  EDIT_PICK_ID_SCALE = BIT(1),
  EDIT_PICK_ID_TRANSLATE = BIT(2),

  EDIT_PICK_ID_ROTATE_X = EDIT_PICK_ID_ROTATE | BIT(3),
  EDIT_PICK_ID_ROTATE_Y = EDIT_PICK_ID_ROTATE | BIT(4),
  EDIT_PICK_ID_ROTATE_Z = EDIT_PICK_ID_ROTATE | BIT(5),

  EDIT_PICK_ID_SCALE_X = EDIT_PICK_ID_SCALE | BIT(3),
  EDIT_PICK_ID_SCALE_Y = EDIT_PICK_ID_SCALE | BIT(4),
  EDIT_PICK_ID_SCALE_Z = EDIT_PICK_ID_SCALE | BIT(5),

  EDIT_PICK_ID_TRANSLATE_X = EDIT_PICK_ID_TRANSLATE | BIT(3),
  EDIT_PICK_ID_TRANSLATE_Y = EDIT_PICK_ID_TRANSLATE | BIT(4),
  EDIT_PICK_ID_TRANSLATE_Z = EDIT_PICK_ID_TRANSLATE | BIT(5)
};

LOCAL_SYM enum edit_error
edit_scale_tool
  (struct edit_imgui* imgui,
   struct edit_model_instance_selection* selection);

LOCAL_SYM enum edit_error
edit_draw_pivot
  (struct app* app,
   const float pos[3],
   const float size,
   const float color[3]);

LOCAL_SYM enum edit_error
edit_draw_rotate_tool
  (struct app* app,
   const float pos[3],
   const float size,
   const float color_x[3],
   const float color_y[3],
   const float color_z[3]);

LOCAL_SYM enum edit_error
edit_draw_scale_tool
  (struct app* app,
   const float pos[3],
   const float size,
   const float color_x[3],
   const float color_y[3],
   const float color_z[3]);

LOCAL_SYM enum edit_error
edit_draw_translate_tool
  (struct app* app,
   const float pos[3],
   const float size,
   const float color_x[3],
   const float color_y[3],
   const float color_z[3]);

#endif /* EDIT_TOOLS_H */

