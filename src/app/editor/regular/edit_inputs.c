#include "app/core/app.h"
#include "app/core/app_cvar.h"
#include "app/core/app_imdraw.h"
#include "app/core/app_view.h"
#include "app/core/app_world.h"
#include "app/editor/regular/edit_context_c.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_inputs.h"
#include "app/editor/edit.h"
#include "sys/mem_allocator.h"
#include "sys/math.h"
#include "window_manager/wm.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_input.h"
#include "window_manager/wm_window.h"
#include <string.h>

struct edit_inputs {
  struct edit_inputs_context {
    struct aosf44 view_transform; /* Must be aligned on 16 Bytes */
    enum edit_transform_space transform_space;
    enum edit_selection_mode selection_mode;
    int entity_transform_flag; /* combination of edit_transform_flag */
    int view_transform_flag; /* combination of edit_transform_flag */

    /* Mouse states */
    struct edit_mouse_state {
      int cursor[2];
      int wheel;
    } mouse;
  } context;

  struct edit_inputs_commands {
    float view_translate[3];
    float view_rotate[3];
  } commands;

  struct app* app;
  struct mem_allocator* allocator;
  struct ref ref;
  float mouse_sensitivity;
  bool is_enabled;
};

#define WORLD_UNIT 1

static void
invoke_picking(struct edit_inputs* inputs);

/*******************************************************************************
 *
 * Input callbacks
 *
 ******************************************************************************/
static void
key_clbk(enum wm_key key, enum wm_state state, void* data)
{
  struct edit_inputs* inputs = data;
  assert(data);

  if(state == WM_RELEASE) {
    switch(key) {
      case WM_KEY_LCTRL:
      case WM_KEY_RCTRL:
      case WM_KEY_LALT:
      case WM_KEY_RALT:
        inputs->context.selection_mode = EDIT_SELECTION_MODE_NEW;
        break;
      default: /* Do nothing */ break;
    }
  } else {
    switch(key) {
      case WM_KEY_R:
        inputs->context.entity_transform_flag = EDIT_TRANSFORM_ROTATE;
        break;
      case WM_KEY_T:
        inputs->context.entity_transform_flag = EDIT_TRANSFORM_TRANSLATE;
        break;
      case WM_KEY_E:
        inputs->context.entity_transform_flag = EDIT_TRANSFORM_SCALE;
        break;
      case WM_KEY_P:
        inputs->context.entity_transform_flag = EDIT_TRANSFORM_NONE;
      case WM_KEY_LCTRL:
      case WM_KEY_RCTRL:
        inputs->context.selection_mode = EDIT_SELECTION_MODE_XOR;
        break;
      case WM_KEY_LALT:
      case WM_KEY_RALT:
        inputs->context.selection_mode = EDIT_SELECTION_MODE_NONE;
        break;
      default: /* Do nothing */ break;
    }
  }
}

static void
mouse_motion_clbk(int x, int y, void* data)
{
  struct edit_inputs* inputs = data;
  struct wm_window* win = NULL;
  assert(data);

  APP(get_active_window(inputs->app, &win));

  if(!win || inputs->context.view_transform_flag == EDIT_TRANSFORM_NONE) {
    inputs->context.mouse.cursor[0] = inputs->context.mouse.cursor[1] = -1;
  } else if (inputs->context.mouse.cursor[0] == -1) {
    inputs->context.mouse.cursor[0] = x;
    inputs->context.mouse.cursor[1] = y;
  } else {
    struct wm_window_desc win_desc;
    const float sensitivity = inputs->mouse_sensitivity;
    float rcp_win_size[2] = { 0.f, 0.f };
    float motion[2] = { 0.f, 0.f };

    WM(get_window_desc(win, &win_desc));
    rcp_win_size[0] = 1.f / (float)win_desc.width;
    rcp_win_size[1] = 1.f / (float)win_desc.height;

    /* Compute the relative movement in the active window. The origin is on the
     * lower left corner of the window. Actually we should have to ensure that
     * the mouse motion is performed in the same active window. */
    motion[0] = (float)(x - inputs->context.mouse.cursor[0]) * rcp_win_size[0];
    motion[1] = (float)(inputs->context.mouse.cursor[1] - y) * rcp_win_size[1];

    if(inputs->context.view_transform_flag & EDIT_TRANSFORM_ROTATE) {
      /* With a sensitivity of 1 the camera rotate of PI when the mouse cross
       * over the entire window. */
      inputs->commands.view_rotate[0] -= PI * motion[1] * sensitivity;
      inputs->commands.view_rotate[1] += PI * motion[0] * sensitivity;
    }
    if(inputs->context.view_transform_flag & EDIT_TRANSFORM_TRANSLATE) {
      /* With a sensitivity of 1 the camera translate of 200 WORLD_UNIT when
       * the mouse cross over the entire window. */
      inputs->commands.view_translate[0] +=
        200 * WORLD_UNIT * motion[0] * sensitivity;
      inputs->commands.view_translate[1] +=
        200 * WORLD_UNIT * motion[1] * sensitivity;
    }
    inputs->context.mouse.cursor[0] = x;
    inputs->context.mouse.cursor[1] = y;
  }
}

static void
mouse_button_clbk(enum wm_mouse_button button, enum wm_state state, void* data)
{
  struct edit_inputs* inputs = data;
  assert(data);

  if(inputs->context.selection_mode != EDIT_SELECTION_MODE_NONE) {
    if(state == WM_PRESS) {
      switch(button) {
        case WM_MOUSE_BUTTON_0:
          invoke_picking(inputs);
          break;
        default: /* Do nothing */ break;
      }
    }
  } else {
    #define SETUP_CAMERA_FLAG(flag) \
    do { \
      if(state == WM_PRESS) inputs->context.view_transform_flag |= flag; \
      else inputs->context.view_transform_flag &= ~flag; \
    } while(0)
    switch(button) {
      case WM_MOUSE_BUTTON_0:
        SETUP_CAMERA_FLAG(EDIT_TRANSFORM_ROTATE);
        break;
      case WM_MOUSE_BUTTON_1:
        SETUP_CAMERA_FLAG(EDIT_TRANSFORM_TRANSLATE);
        break;
      default: /* Do nothing */ break;
    }
    #undef SETUP_CAMERA_FLAG
  }
}

static void
mouse_wheel_clbk(int pos, void* data)
{
  struct edit_inputs* inputs = data;
  float sensitivity = 0.f;
  int zoom = 0;
  assert(data);

  if(inputs->context.selection_mode != EDIT_SELECTION_MODE_NONE)
    return;

  sensitivity = inputs->mouse_sensitivity;
  zoom = pos - inputs->context.mouse.wheel;
  if(zoom != 0) {
    inputs->context.mouse.wheel = pos;
    /* With a sensitivity of 1 the camera zoom of 10 WORLD_UNIT per wheel pos */
    inputs->commands.view_translate[2] += zoom * 10 * WORLD_UNIT * sensitivity;
  }
}

/*******************************************************************************
 *
 * Helper functions
 *
 ******************************************************************************/
static void
invoke_picking(struct edit_inputs* inputs)
{
  struct wm_device* wm = NULL;
  struct app_view* view = NULL;
  struct app_world* world = NULL;
  int x = 0, y = 0;

  assert(inputs);
  APP(get_window_manager_device(inputs->app, &wm));
  APP(get_main_world(inputs->app, &world));
  APP(get_main_view(inputs->app, &view));
  WM(get_mouse_position(wm, &x, &y));

  APP(clear_picking(inputs->app));
  APP(world_pick
    (world,
     view,
     (const unsigned int[]){x, y},
     (const unsigned int[]){1, 1}));
  APP(imdraw_pick
    (inputs->app,
     (const unsigned int[]){x, y},
     (const unsigned int[]){1, 1}));
}

static void
release_inputs(struct ref* ref)
{
  struct edit_inputs* inputs = NULL;
  inputs = CONTAINER_OF(ref, struct edit_inputs, ref);
  if(inputs->app) {
    EDIT(inputs_disable(inputs));
    APP(ref_put(inputs->app));
  }

  MEM_FREE(inputs->allocator, inputs);
}

static void
flush_commands(struct edit_inputs* inputs)
{
  assert(inputs);

  /* Flush view_translate command */
  if(inputs->commands.view_translate[0] != 0.f
  || inputs->commands.view_translate[1] != 0.f
  || inputs->commands.view_translate[2] != 0.f) {
    /* Locally translate the camera. */
    inputs->context.view_transform.c3 = vf4_add
      (inputs->context.view_transform.c3,
       vf4_set
       (inputs->commands.view_translate[0],
        inputs->commands.view_translate[1],
        inputs->commands.view_translate[2],
        0.f));
  }

  /* Flush view_rotate command */
  if(inputs->commands.view_rotate[0] != 0.f
  || inputs->commands.view_rotate[1] != 0.f
  || inputs->commands.view_rotate[2] != 0.f) {
    struct aosf33 pitch_roll_rot, yaw_rot, rot, inv_rot;
    vf4_t view_pos;

    /* Negate the rotation angle around the Y axis if the up vector of the view
     * transform point downward. */
    if(vf4_y(inputs->context.view_transform.c1) < 0.f) {
      inputs->commands.view_rotate[1] = -inputs->commands.view_rotate[1];
    }
    /* Compute rotations matrices. */
    aosf33_yaw_rotation(&yaw_rot, inputs->commands.view_rotate[1]);
    aosf33_rotation
      (&pitch_roll_rot,
       inputs->commands.view_rotate[0], 0.f, inputs->commands.view_rotate[2]);
    /* Retrieve the position of the camera in world space. */
    rot.c0 = inputs->context.view_transform.c0;
    rot.c1 = inputs->context.view_transform.c1;
    rot.c2 = inputs->context.view_transform.c2;
    aosf33_inverse(&inv_rot, &rot);
    view_pos = aosf33_mulf3(&inv_rot, inputs->context.view_transform.c3);
    /* Compute the new rotation matrix. */
    aosf33_mulf33(&rot, &pitch_roll_rot, &rot);
    aosf33_mulf33(&rot, &rot, &yaw_rot);
    inputs->context.view_transform.c0 = rot.c0;
    inputs->context.view_transform.c1 = rot.c1;
    inputs->context.view_transform.c2 = rot.c2;
    /* Set the view position with respect to the new rotation matrix. */
    view_pos = aosf33_mulf3(&rot, view_pos);
    inputs->context.view_transform.c3 = vf4_xyzd(view_pos, vf4_set1(1.f));
  }

  /* Reset commands. */
  memset(&inputs->commands, 0, sizeof(struct edit_inputs_commands));
}


/*******************************************************************************
 *
 * Input functions
 *
 ******************************************************************************/
enum edit_error
edit_create_inputs
  (struct app* app,
   struct mem_allocator* allocator,
   struct edit_inputs** out_inputs)
{
  const struct aosf44* f44 = NULL;
  struct app_view* main_view = NULL;
  struct edit_inputs* inputs = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;

  if(UNLIKELY(!app || !allocator || !out_inputs)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  inputs = MEM_ALIGNED_ALLOC(allocator, sizeof(struct edit_inputs), 16);
  if(!inputs) {
    edit_err = EDIT_MEMORY_ERROR;
    goto error;
  }
  assert(IS_ALIGNED(&inputs->context.view_transform, 16));
  memset(inputs, 0, sizeof(struct edit_inputs));
  ref_init(&inputs->ref);
  inputs->allocator = allocator;

  /* Init the view state from the app main view. */
  APP(get_main_view(app, &main_view));
  APP(get_raw_view_transform(main_view, &f44));
  inputs->context.view_transform = *f44;

  /* Default mouse sensitivity */
  inputs->mouse_sensitivity = 1.f;

  /* Take the owner ship onto the app */
  APP(ref_get(app));
  inputs->app = app;

exit:
  if(out_inputs)
    *out_inputs = inputs;
  return edit_err;
error:
  if(inputs) {
    EDIT(inputs_ref_put(inputs));
    inputs = NULL;
  }
  goto exit;
}

enum edit_error
edit_inputs_ref_get(struct edit_inputs* inputs)
{
  if(UNLIKELY(!inputs))
    return EDIT_INVALID_ARGUMENT;
  ref_get(&inputs->ref);
  return EDIT_NO_ERROR;
}

enum edit_error
edit_inputs_ref_put(struct edit_inputs* inputs)
{
  if(UNLIKELY(!inputs))
    return EDIT_INVALID_ARGUMENT;
  ref_put(&inputs->ref, release_inputs);
  return EDIT_NO_ERROR;
}

enum edit_error
edit_inputs_enable(struct edit_inputs* inputs)
{
  struct wm_device* wm = NULL;

  if(UNLIKELY(!inputs))
    return EDIT_INVALID_ARGUMENT;

  if(inputs->is_enabled == true)
    return EDIT_NO_ERROR;

  APP(get_window_manager_device(inputs->app, &wm));

  #define ATTACH_CLBK(name) \
    bool is_##name##_clbk_attached = false; \
    do { \
      WM(is_##name##_callback_attached \
         (wm, name##_clbk, inputs, &is_##name##_clbk_attached)); \
      if(!is_##name##_clbk_attached) { \
        WM(attach_##name##_callback(wm, name##_clbk, inputs)); \
      } \
    } while(0)
  ATTACH_CLBK(key);
  ATTACH_CLBK(mouse_motion);
  ATTACH_CLBK(mouse_button);
  ATTACH_CLBK(mouse_wheel);
  #undef ATTACH_CLBK

  /* Retrieve the current mouse states */
  WM(get_mouse_wheel(wm, &inputs->context.mouse.wheel));
  WM(get_mouse_position
    (wm, inputs->context.mouse.cursor + 0, inputs->context.mouse.cursor + 1));

  inputs->is_enabled = true;

  return EDIT_NO_ERROR;
}

enum edit_error
edit_inputs_disable(struct edit_inputs* inputs)
{
  struct wm_device* wm = NULL;

  if(UNLIKELY(!inputs))
    return EDIT_INVALID_ARGUMENT;

  if(inputs->is_enabled == false)
    return EDIT_NO_ERROR;

  APP(get_window_manager_device(inputs->app, &wm));

  #define DETACH_CLBK(name) \
    bool is_##name##_clbk_attached = false; \
    do { \
      WM(is_##name##_callback_attached \
         (wm, name##_clbk, inputs, &is_##name##_clbk_attached)); \
      if(is_##name##_clbk_attached) { \
        WM(detach_##name##_callback(wm, name##_clbk, inputs)); \
      } \
    } while(0)
  DETACH_CLBK(key);
  DETACH_CLBK(mouse_motion);
  DETACH_CLBK(mouse_button);
  DETACH_CLBK(mouse_wheel);
  #undef DETACH_CLBK

  inputs->is_enabled = false;

  return EDIT_NO_ERROR;
}

enum edit_error
edit_inputs_set_mouse_sensitivity
  (struct edit_inputs* input,
   const float sensitivity)
{
  if(UNLIKELY(!input))
    return EDIT_INVALID_ARGUMENT;
  input->mouse_sensitivity = sensitivity;
  return EDIT_NO_ERROR;
}

enum edit_error
edit_inputs_flush(struct edit_inputs* input)
{
  if(UNLIKELY(!input))
    return EDIT_INVALID_ARGUMENT;
  flush_commands(input);
  return EDIT_NO_ERROR;
}

enum edit_error
edit_inputs_get_state
  (const struct edit_inputs* input,
   struct edit_inputs_state* state)
{
  if(UNLIKELY(!input || !state))
    return EDIT_INVALID_ARGUMENT;
  state->view_transform = input->context.view_transform;
  state->selection_mode = input->context.selection_mode;
  state->entity_transform_flag = input->context.entity_transform_flag;
  state->is_enabled = input->is_enabled;
  return EDIT_NO_ERROR;
}

