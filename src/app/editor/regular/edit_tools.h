#ifndef EDIT_TOOLS_H
#define EDIT_TOOLS_H

#include "app/editor/edit_error.h"
#include "sys/sys.h"

struct app;
struct edit_imgui;
struct edit_model_instance_selection;

LOCAL_SYM enum edit_error
edit_scale_tool
  (struct edit_imgui* imgui,
   struct edit_model_instance_selection* selection,
   const float sensitivity);

LOCAL_SYM enum edit_error
edit_translate_tool
  (struct edit_imgui* imgui,
   struct edit_model_instance_selection* selection,
   const float sensitivity);

LOCAL_SYM enum edit_error
edit_rotate_tool
  (struct edit_imgui* imgui,
   struct edit_model_instance_selection* selection,
   const float sensitivity);

LOCAL_SYM enum edit_error
edit_draw_pivot
  (struct app* app,
   const float pos[3],
   const float size,
   const float color[3]);

#endif /* EDIT_TOOLS_H */

