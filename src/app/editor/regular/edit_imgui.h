#ifndef EDIT_IMGUI_H
#define EDIT_IMGUI_H

#include "app/editor/edit_error.h"
#include "sys/sys.h"

#define EDIT_IMGUI_ID COUNTER

struct app;
struct mem_allocator;
struct edit_imgui;

LOCAL_SYM enum edit_error
edit_create_imgui
  (struct app* app,
   struct mem_allocator* allocator,
   struct edit_imgui** imgui);

LOCAL_SYM enum edit_error
edit_imgui_ref_get
  (struct edit_imgui* imgui);

LOCAL_SYM enum edit_error
edit_imgui_ref_put
  (struct edit_imgui* imgui);

/* Synchronise the imgui state with inputs */
LOCAL_SYM enum edit_error
edit_imgui_sync
  (struct edit_imgui* imgui);

LOCAL_SYM enum edit_error
edit_imgui_enable_item
  (struct edit_imgui* imgui,
   const uint32_t item_id);

LOCAL_SYM enum edit_error
edit_imgui_rotate_tool
  (struct edit_imgui* imgui,
   const uint32_t id,
   const float pos[3],
   const float size,
   const float color_x[3],
   const float color_y[3],
   const float color_z[3],
   int rotation[3]);

LOCAL_SYM enum edit_error
edit_imgui_scale_tool
  (struct edit_imgui* imgui,
   const uint32_t id,
   const float pos[3],
   const float size,
   const float color_x[3],
   const float color_y[3],
   const float color_z[3],
   int scale[3]);

LOCAL_SYM enum edit_error
edit_imgui_translate_tool
  (struct edit_imgui* imgui,
   const uint32_t id,
   const float pos[3],
   const float size,
   const float color_x[3],
   const float color_y[3],
   const float color_z[3],
   int translation[3]);

#endif /* EDIT_IMGUI_H */

