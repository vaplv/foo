#include "app/core/app.h"
#include "app/core/app_core.h"
#include "app/core/app_error.h"
#include "app/core/app_imdraw.h"
#include "app/core/app_view.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_imgui.h"
#include "app/editor/edit.h"
#include "maths/simd/aosf44.h"
#include "stdlib/sl_hash_table.h"
#include "sys/ref_count.h"
#include "sys/math.h"
#include "sys/mem_allocator.h"
#include "window_manager/wm.h"
#include "window_manager/wm_input.h"
#include <string.h>

#define IMGUI_ITEM_NULL UINT32_MAX

struct imgui_item_data {
  uint16_t x;
  uint16_t y;
};

struct imgui_state {
  int mouse_x;
  int mouse_y;
  int mouse_z;
  enum wm_state mouse_button;
};

struct edit_imgui {
  struct app* app;
  struct mem_allocator* allocator;
  struct sl_hash_table* item_htbl;
  struct ref ref;
  struct imgui_state state;
  uint32_t enabled_item;
};

/*******************************************************************************
 *
 * Helper functions
 *
 ******************************************************************************/
static size_t
hash_key( const void* key )
{
  return sl_hash(key, sizeof(uint32_t));
}

static bool
eq_key( const void* a, const void* b )
{
  return *(int*)a == *(int*)b;
}

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

static int
setup_tool_item
  (struct sl_hash_table* item_htbl,
   const struct aosf44* view_proj,
   const vf4_t axis,
   const uint32_t item_id,
   const struct imgui_item_data* data,
   const bool invoke)
{
  struct imgui_item_data* item = NULL;
  int result = 0;

  assert(item_htbl && view_proj && data);

  SL(hash_table_find(item_htbl, &item_id, (void**)&item));
  if(invoke == false) {
    if(item != NULL) {
      SL(hash_table_erase(item_htbl, &item_id, NULL));
    }
  } else {
    if(item == NULL) {
      SL(hash_table_insert(item_htbl, &item_id, data));
    } else {
      const vf4_t vec3_axis = aosf44_mulf4(view_proj, axis);
      const vf4_t vec2_axis = vf4_xycw(vec3_axis, vf4_zero());
      const vf4_t vec2 = vf4_set(data->x - item->x, item->y - data->y, 0.f,0.f);
      const vf4_t u = vf4_dot2(vec2_axis, vec2);
      const vf4_t vec2_item = vf4_mul(vf4_normalize2(vec2_axis), u);
      const vf4_t len = vf4_len2(vec2_item);
      const vf4_t sign = vf4_dot2(vec2_item, vec2_axis);

      result = vf4_x(vf4_sel(len, vf4_minus(len), vf4_lt(sign, vf4_zero())));
      *item = *data;
    }
  }
  return result;
}

static void
release_imgui(struct ref* ref)
{
  struct edit_imgui* imgui = NULL;
  imgui = CONTAINER_OF(ref, struct edit_imgui, ref);

  if(imgui->item_htbl)
    SL(free_hash_table(imgui->item_htbl));
  if(imgui->app)
    APP(ref_put(imgui->app));

  MEM_FREE(imgui->allocator, imgui);
}

/*******************************************************************************
 *
 * IMGUI functions
 *
 ******************************************************************************/
enum edit_error
edit_create_imgui
  (struct app* app,
   struct mem_allocator* allocator,
   struct edit_imgui** out_imgui)
{
  struct edit_imgui* imgui = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(UNLIKELY(!app || !allocator || !out_imgui)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  imgui = MEM_CALLOC(allocator, 1, sizeof(struct edit_imgui));
  if(!imgui) {
    edit_err = EDIT_MEMORY_ERROR;
    goto error;
  }
  ref_init(&imgui->ref);
  imgui->allocator = allocator;
  APP(ref_get(app));
  imgui->app = app;
  imgui->enabled_item = IMGUI_ITEM_NULL;

  sl_err = sl_create_hash_table
    (sizeof(uint32_t),
     ALIGNOF(uint32_t),
     sizeof(struct imgui_item_data),
     ALIGNOF(struct imgui_item_data),
     hash_key,
     eq_key,
     allocator,
     &imgui->item_htbl);
  if(sl_err != SL_NO_ERROR) {
    edit_err = sl_to_edit_error(sl_err);
    goto error;
  }

exit:
  if(out_imgui)
    *out_imgui = imgui;
  return edit_err;
error:
  if(imgui) {
    EDIT(imgui_ref_put(imgui));
    imgui = NULL;
  }
  goto exit;
}

enum edit_error
edit_imgui_ref_get(struct edit_imgui* imgui)
{
  if(UNLIKELY(!imgui))
    return EDIT_INVALID_ARGUMENT;
  ref_get(&imgui->ref);
  return EDIT_NO_ERROR;
}

enum edit_error
edit_imgui_ref_put(struct edit_imgui* imgui)
{
  if(UNLIKELY(!imgui))
    return EDIT_INVALID_ARGUMENT;
  ref_put(&imgui->ref, release_imgui);
  return EDIT_NO_ERROR;
}

enum edit_error
edit_imgui_sync(struct edit_imgui* imgui)
{
  struct wm_device* wm = NULL;

  if(UNLIKELY(!imgui))
    return EDIT_INVALID_ARGUMENT;

  APP(get_window_manager_device(imgui->app, &wm));
  WM(get_mouse_position(wm, &imgui->state.mouse_x, &imgui->state.mouse_y));
  WM(get_mouse_wheel(wm, &imgui->state.mouse_z));
  WM(get_mouse_button_state(wm, WM_MOUSE_BUTTON_0, &imgui->state.mouse_button));
  return EDIT_NO_ERROR;
}

enum edit_error
edit_imgui_enable_item(struct edit_imgui* imgui, const uint32_t id)
{
  if(!imgui || id == IMGUI_ITEM_NULL)
    return EDIT_INVALID_ARGUMENT;
  imgui->enabled_item = id;
  return EDIT_NO_ERROR;
}

enum edit_error
edit_imgui_rotate_tool
  (struct edit_imgui* imgui,
   const uint32_t id,
   const float pos[3],
   const float size,
   const float color_x[3],
   const float color_y[3],
   const float color_z[3],
   float rotate[3])
{
  uint32_t id_rot_xyz = 0;
  uint32_t id_rot_x = 0;
  uint32_t id_rot_y = 0;
  uint32_t id_rot_z = 0;

  if(UNLIKELY(!imgui || !pos || !color_x || !color_y || !color_z || !rotate))
    return EDIT_INVALID_ARGUMENT;

  if(UNLIKELY(id + 3 > APP_PICK_ID_MAX))
    return EDIT_MEMORY_ERROR;

  rotate[0] = rotate[1] = rotate[2] = 0.f;

  id_rot_xyz = id + 0;
  id_rot_x = id + 1;
  id_rot_y = id + 2;
  id_rot_z = id + 3;

  if(imgui->enabled_item == id_rot_xyz
  || imgui->enabled_item == id_rot_x
  || imgui->enabled_item == id_rot_y
  || imgui->enabled_item == id_rot_z) {
    struct aosf44 view_proj;
    struct imgui_item_data new_data;
    struct app_view* view = NULL;
    const float rotate_factor = 0.01f;
    bool mouse_pressed = false;
    memset(&new_data, 0, sizeof(struct imgui_item_data));

    mouse_pressed = imgui->state.mouse_button == WM_PRESS;
    new_data.x = imgui->state.mouse_x;
    new_data.y = imgui->state.mouse_y;

    APP(get_main_view(imgui->app, &view));
    APP(get_view_proj_matrix(view, &view_proj));

    if(imgui->enabled_item==id_rot_x || imgui->enabled_item==id_rot_xyz) {
      rotate[0] = setup_tool_item
        (imgui->item_htbl,
         &view_proj,
         vf4_set(0.f, 1.f, 0.f, 0.f),
         id_rot_x,
         &new_data,
         mouse_pressed);
      rotate[0] *= rotate_factor;
    }
    if(imgui->enabled_item==id_rot_y || imgui->enabled_item==id_rot_xyz) {
      rotate[1] = setup_tool_item
        (imgui->item_htbl,
         &view_proj,
         vf4_set(0.f, 0.f, 1.f, 0.f),
         id_rot_y,
         &new_data,
         mouse_pressed);
      rotate[1] *= rotate_factor;
    }
    if(imgui->enabled_item==id_rot_z || imgui->enabled_item==id_rot_xyz) {
      rotate[2] = setup_tool_item
        (imgui->item_htbl,
         &view_proj,
         vf4_set(1.f, 0.f, 0.f, 0.f),
         id_rot_z,
         &new_data,
         mouse_pressed);
      rotate[2] *= rotate_factor;
    }
    if(mouse_pressed == false) {
      imgui->enabled_item = IMGUI_ITEM_NULL;
    }
  }

  /* Invoke rotate tool rendering */
  draw_basis_circles
    (imgui->app, pos, size,
     color_y, color_z, color_x,
     APP_PICK_NONE, APP_PICK_NONE, APP_PICK_NONE);
  draw_basis
    (imgui->app, pos, size,
     APP_IM_VECTOR_CUBE_MARKER,
     color_y, color_z, color_x,
     id_rot_xyz, id_rot_y, id_rot_z, id_rot_x);

  return EDIT_NO_ERROR;

}

enum edit_error
edit_imgui_scale_tool
  (struct edit_imgui* imgui,
   const uint32_t id,
   const float pos[3],
   const float size,
   const float color_x[3],
   const float color_y[3],
   const float color_z[3],
   float scale[3])
{
  uint32_t id_scale_xyz = 0;
  uint32_t id_scale_x = 0;
  uint32_t id_scale_y = 0;
  uint32_t id_scale_z = 0;

  if(UNLIKELY(!imgui || !pos || !color_x || !color_y || !color_z || !scale))
    return EDIT_INVALID_ARGUMENT;

  if(UNLIKELY(id + 3 > APP_PICK_ID_MAX))
    return EDIT_MEMORY_ERROR;

  scale[0] = scale[1] = scale[2] = 1.f;

  id_scale_xyz = id + 0;
  id_scale_x = id + 1;
  id_scale_y = id + 2;
  id_scale_z = id + 3;

  if(imgui->enabled_item == id_scale_xyz
  || imgui->enabled_item == id_scale_x
  || imgui->enabled_item == id_scale_y
  || imgui->enabled_item == id_scale_z) {
    struct aosf44 transform;
    struct imgui_item_data new_data;
    struct app_view* view = NULL;
    const float scale_factor = 0.01f;
    bool mouse_pressed = false;
    memset(&new_data, 0, sizeof(struct imgui_item_data));

    mouse_pressed = imgui->state.mouse_button == WM_PRESS;
    new_data.x = imgui->state.mouse_x;
    new_data.y = imgui->state.mouse_y;

    if(imgui->enabled_item != id_scale_xyz) {
      APP(get_main_view(imgui->app, &view));
      APP(get_view_proj_matrix(view, &transform));
    } else {
      aosf44_identity(&transform);
    }

    if(imgui->enabled_item == id_scale_x) {
      scale[0] = setup_tool_item
        (imgui->item_htbl,
         &transform,
         vf4_set(1.f, 0.f, 0.f, 0.f),
         id_scale_x,
         &new_data,
         mouse_pressed);
      scale[0] = scale[0] * scale_factor;
      scale[0] = scale[0] < 0 ? 1.f / (1.f - scale[0]) : 1.f + scale[0];
    }
    if(imgui->enabled_item == id_scale_y) {
      scale[1] = setup_tool_item
        (imgui->item_htbl,
         &transform,
         vf4_set(0.f, 1.f, 0.f, 0.f),
         id_scale_y,
         &new_data,
         mouse_pressed);
      scale[1] = scale[1] * scale_factor;
      scale[1] = scale[1] < 0 ? 1.f / (1.f - scale[1]) : 1.f + scale[1];
    }
    if(imgui->enabled_item == id_scale_z) {
      scale[2] = setup_tool_item
        (imgui->item_htbl,
         &transform,
         vf4_set(0.f, 0.f, 1.f, 0.f),
         id_scale_z,
         &new_data,
         mouse_pressed);
      scale[2] = scale[2] * scale_factor;
      scale[2] = scale[2] < 0 ? 1.f / (1.f - scale[2]) : 1.f + scale[2];
    }
    if(imgui->enabled_item == id_scale_xyz) {
      float scale_xyz = setup_tool_item
        (imgui->item_htbl,
         &transform,
         vf4_normalize3(vf4_set(1.f, 1.f, 0.f, 0.f)),
         id_scale_xyz,
         &new_data,
         mouse_pressed);
      scale_xyz = scale_xyz * scale_factor;
      scale_xyz = scale_xyz < 0 ? 1.f / (1.f - scale_xyz) : 1.f + scale_xyz;
      scale[0] = scale[1] = scale[2] = scale_xyz;
    }
    if(mouse_pressed == false) {
      imgui->enabled_item = IMGUI_ITEM_NULL;
    }
  }

  /* Invoke scale tool rendering */
  draw_basis
    (imgui->app, pos, size,
     APP_IM_VECTOR_CUBE_MARKER,
     color_x, color_y, color_z,
     id_scale_xyz, id_scale_x, id_scale_y, id_scale_z);

  return EDIT_NO_ERROR;
}

enum edit_error
edit_imgui_translate_tool
  (struct edit_imgui* imgui,
   const uint32_t id,
   const float pos[3],
   const float size,
   const float color_x[3],
   const float color_y[3],
   const float color_z[3],
   float translate[3])
{
  uint32_t id_trans_xyz = 0;
  uint32_t id_trans_x = 0;
  uint32_t id_trans_y = 0;
  uint32_t id_trans_z = 0;

  if(UNLIKELY(!imgui || !pos || !color_x || !color_y || !color_z || !translate))
    return EDIT_INVALID_ARGUMENT;

  if(UNLIKELY(id + 3 > APP_PICK_ID_MAX))
    return EDIT_MEMORY_ERROR;

  translate[0] = translate[1] = translate[2] = 0.f;

  id_trans_xyz = id + 0;
  id_trans_x = id + 1;
  id_trans_y = id + 2;
  id_trans_z = id + 3;

  if(imgui->enabled_item == id_trans_xyz
  || imgui->enabled_item == id_trans_x
  || imgui->enabled_item == id_trans_y
  || imgui->enabled_item == id_trans_z) {
    struct aosf44 view_proj;
    struct imgui_item_data new_data;
    struct app_view* view = NULL;
    const float translate_factor = 0.1f;
    bool mouse_pressed = false;
    memset(&new_data, 0, sizeof(struct imgui_item_data));

    mouse_pressed = imgui->state.mouse_button == WM_PRESS;
    new_data.x = imgui->state.mouse_x;
    new_data.y = imgui->state.mouse_y;

    APP(get_main_view(imgui->app, &view));
    APP(get_view_proj_matrix(view, &view_proj));

    if(imgui->enabled_item==id_trans_x || imgui->enabled_item==id_trans_xyz) {
      translate[0] = setup_tool_item
        (imgui->item_htbl,
         &view_proj,
         vf4_set(1.f, 0.f, 0.f, 0.f),
         id_trans_x,
         &new_data,
         mouse_pressed);
      translate[0] *= translate_factor;
    }
    if(imgui->enabled_item==id_trans_y || imgui->enabled_item==id_trans_xyz) {
      translate[1] = setup_tool_item
        (imgui->item_htbl,
         &view_proj,
         vf4_set(0.f, 1.f, 0.f, 0.f),
         id_trans_y,
         &new_data,
         mouse_pressed);
      translate[1] *= translate_factor;
    }
    if(imgui->enabled_item==id_trans_z || imgui->enabled_item==id_trans_xyz) {
      translate[2] = setup_tool_item
        (imgui->item_htbl,
         &view_proj,
         vf4_set(0.f, 0.f, 1.f, 0.f),
         id_trans_z,
         &new_data,
         mouse_pressed);
      translate[2] *= translate_factor;
    }
    if(mouse_pressed == false) {
      imgui->enabled_item = IMGUI_ITEM_NULL;
    }
  }

  /* Invoke translate tool rendering */
  draw_basis
    (imgui->app,
     (float[]){pos[0]+translate[0], pos[1]+translate[1], pos[2]+translate[2]},
     size,
     APP_IM_VECTOR_CONE_MARKER,
     color_x, color_y, color_z,
     id_trans_xyz, id_trans_x, id_trans_y, id_trans_z);

  return EDIT_NO_ERROR;
}

#undef IMGUI_ITEM_NULL
