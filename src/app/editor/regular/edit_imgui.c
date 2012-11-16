#include "app/core/app.h"
#include "app/core/app_core.h"
#include "app/core/app_error.h"
#include "app/core/app_imdraw.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_imgui.h"
#include "app/editor/edit.h"
#include "stdlib/sl_hash_table.h"
#include "sys/ref_count.h"
#include "sys/math.h"
#include "sys/mem_allocator.h"
#include "window_manager/wm.h"
#include "window_manager/wm_input.h"

struct imitem {
  bool is_enabled;
  int val;
};

struct edit_imgui {
  struct app* app;
  struct mem_allocator* allocator;
  struct sl_hash_table* imitem_htbl;
  struct ref ref;
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

static void
release_imgui(struct ref* ref)
{
  struct edit_imgui* imgui = NULL;
  imgui = CONTAINER_OF(ref, struct edit_imgui, ref);

  if(imgui->imitem_htbl)
    SL(free_hash_table(imgui->imitem_htbl));
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

  sl_err = sl_create_hash_table
    (sizeof(uint32_t),
     ALIGNOF(uint32_t),
     sizeof(struct imitem),
     ALIGNOF(struct imitem),
     hash_key,
     eq_key,
     allocator,
     &imgui->imitem_htbl);
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
edit_imgui_enable_item(struct edit_imgui* imgui, const uint32_t id)
{
  struct imitem* imitem = NULL;
  struct wm_device* wm = NULL;

  if(!imgui || id >= APP_PICK_ID_MAX)
    return EDIT_INVALID_ARGUMENT;

  /* APP(get_window_manager_device(imgui->app, &wm));
  WM(get_mouse_position(wm, &x, &y));
  WM(get_mouse_wheel(wm, &z));
   TODO 
   */
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
   int scale[3])
{
  struct imitem* imitem = NULL;
  struct wm_device* wm = NULL;
  int x = 0, y = 0, z = 0;
  uint32_t id_scale_x = 0;
  uint32_t id_scale_y = 0;
  uint32_t id_scale_z = 0;

  if(UNLIKELY
  (  !imgui
  || !pos
  || !color_x
  || !color_y
  || !color_z
  || !scale))
    return EDIT_INVALID_ARGUMENT;

  if(UNLIKELY(id + 2 > APP_PICK_ID_MAX))
    return EDIT_MEMORY_ERROR;

  /* Retrieve the mouse position */
  APP(get_window_manager_device(imgui->app, &wm));
  WM(get_mouse_position(wm, &x, &y));
  WM(get_mouse_wheel(wm, &z));

  /* Invoke scale tool rendering */
  id_scale_x = id + 0;
  id_scale_y = id + 1;
  id_scale_z = id + 2;
  draw_basis
    (imgui->app,
     pos,
     size,
     APP_IM_VECTOR_CUBE_MARKER,
     color_x,
     color_y,
     color_z,
     APP_PICK_ID_MAX,
     id_scale_x,
     id_scale_y,
     id_scale_z);

  /* Retrieve the scale values */
  SL(hash_table_find(imgui->imitem_htbl, &id_scale_x, (void**)&imitem));
  if(imitem != NULL && imitem->is_enabled) {
    scale[0] = x - imitem->val;
    imitem->val = x;
  }
  SL(hash_table_find(imgui->imitem_htbl, &id_scale_y, (void**)&imitem));
  if(imitem != NULL && imitem->is_enabled) {
    scale[1] = y - imitem->val;
    imitem->val = y;
  }
  SL(hash_table_find(imgui->imitem_htbl, &id_scale_z, (void**)&imitem));
  if(imitem != NULL && imitem->is_enabled) {
    scale[2] = z - imitem->val;
    imitem->val = z;
  }
  return EDIT_NO_ERROR;
}

