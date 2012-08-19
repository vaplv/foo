#include "app/core/app.h"
#include "app/core/app_cvar.h"
#include "app/core/app_view.h"
#include "app/editor/regular/edit_context_c.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_inputs.h"
#include "sys/math.h"
#include "window_manager/wm.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_input.h"
#include "window_manager/wm_window.h"
#include <string.h>

#define WORLD_UNIT 1

/*******************************************************************************
 *
 * Helper functions
 *
 ******************************************************************************/
static void
key_clbk(enum wm_key key, enum wm_state state, void* data)
{
  struct edit_context* ctxt = data;
  assert(data);

  if(state != WM_PRESS)
    return;

  switch(key) {
    case WM_KEY_R:
      ctxt->states.entity_transform_flag = EDIT_TRANSFORM_ROTATE;
      break;
    case WM_KEY_T:
      ctxt->states.entity_transform_flag = EDIT_TRANSFORM_TRANSLATE;
      break;
    case WM_KEY_E:
      ctxt->states.entity_transform_flag = EDIT_TRANSFORM_SCALE;
      break;
    case WM_KEY_P:
      ctxt->states.entity_transform_flag = EDIT_TRANSFORM_NONE;
      break;
    default:
      /* Do nothing */
      break;
  }
}

static void
mouse_motion_clbk(int x, int y, void* data)
{
  struct edit_context* ctxt = data;
  struct wm_window* win = NULL;
  assert(ctxt);

  APP(get_active_window(ctxt->app, &win));

  if(!win || ctxt->states.view_transform_flag == EDIT_TRANSFORM_NONE) {
    ctxt->states.mouse_cursor[0] = ctxt->states.mouse_cursor[1] = -1;
  } else if (ctxt->states.mouse_cursor[0] == -1) {
    ctxt->states.mouse_cursor[0] = x;
    ctxt->states.mouse_cursor[1] = y;
  } else {
    struct wm_window_desc win_desc;
    const float sensitivity = ctxt->cvars.mouse_sensitivity->value.real;
    float rcp_win_size[2] = { 0.f, 0.f };
    float motion[2] = { 0.f, 0.f };

    WM(get_window_desc(win, &win_desc));
    rcp_win_size[0] = 1.f / (float)win_desc.width;
    rcp_win_size[1] = 1.f / (float)win_desc.height;

    /* Compute the relative movement in the active window. The origin is on the
     * lower left corner of the window. Actually we should have to ensure that
     * the mouse motion is performed in the same active window. */
    motion[0] = (float)(x - ctxt->states.mouse_cursor[0]) * rcp_win_size[0];
    motion[1] = (float)(ctxt->states.mouse_cursor[1] - y) * rcp_win_size[1];

    if(ctxt->states.view_transform_flag & EDIT_TRANSFORM_ROTATE) {
      /* With a sensitivity of 1 the camera rotate of PI when the mouse cross
       * over the entire window. */
      ctxt->inputs.view_rotation[0] -= PI * motion[1] * sensitivity;
      ctxt->inputs.view_rotation[1] += PI * motion[0] * sensitivity;
    }
    if(ctxt->states.view_transform_flag & EDIT_TRANSFORM_TRANSLATE) {
      /* With a sensitivity of 1 the camera translate of 200 WORLD_UNIT when
       * the mouse cross over the entire window. */
      ctxt->inputs.view_translation[0] +=
        200 * WORLD_UNIT * motion[0] * sensitivity;
      ctxt->inputs.view_translation[1] +=
        200 * WORLD_UNIT * motion[1] * sensitivity;
    }
    ctxt->states.mouse_cursor[0] = x;
    ctxt->states.mouse_cursor[1] = y;
    ctxt->inputs.updated = true;
  }
}

static void
mouse_button_clbk(enum wm_mouse_button button, enum wm_state state, void* data)
{
  struct edit_context* ctxt = data;
  assert(ctxt);

  #define SETUP_CAMERA_FLAG(flag) \
    do { \
      if(state == WM_PRESS) ctxt->states.view_transform_flag |= flag; \
      else ctxt->states.view_transform_flag &= ~flag; \
    } while(0)
  switch(button) {
    case WM_MOUSE_BUTTON_0: SETUP_CAMERA_FLAG(EDIT_TRANSFORM_ROTATE); break;
    case WM_MOUSE_BUTTON_1: SETUP_CAMERA_FLAG(EDIT_TRANSFORM_TRANSLATE); break;
    default: /* Do nothing */ break;
  }
  #undef SETUP_CAMERA_FLAG
}

static void
mouse_wheel_clbk(int pos, void* data)
{
  struct edit_context* ctxt = data;
  float sensitivity = 0.f;
  int zoom = 0;
  assert(ctxt);

  sensitivity = ctxt->cvars.mouse_sensitivity->value.real;
  zoom = pos - ctxt->states.mouse_wheel;
  if(zoom != 0) {
    ctxt->states.mouse_wheel = pos;
    ctxt->inputs.updated = true;

    /* With a sensitivity of 1 the camera zoom of 10 WORLD_UNIT per wheel pos. */
    ctxt->inputs.view_translation[2] += zoom * 10 * WORLD_UNIT * sensitivity;
  }
}

/*******************************************************************************
 *
 * Input functions
 *
 ******************************************************************************/
enum edit_error
edit_init_inputs(struct edit_context* ctxt)
{
  const struct aosf44* f44 = NULL;
  struct app_view* main_view = NULL;
  struct wm_device* wm = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;

  if(UNLIKELY(!ctxt)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }
  APP(get_window_manager_device(ctxt->app, &wm));

  #define ATTACH_CLBK(name) \
    bool is_##name##_clbk_attached = false; \
    do { \
      WM(is_##name##_callback_attached \
        (wm, name##_clbk, ctxt, &is_##name##_clbk_attached)); \
      if(!is_##name##_clbk_attached) { \
        wm_err = wm_attach_##name##_callback(wm, name##_clbk, ctxt); \
        if(wm_err != WM_NO_ERROR) { \
          edit_err = wm_to_edit_error(wm_err); \
          goto error; \
        } \
      } \
    } while(0)
  ATTACH_CLBK(key);
  ATTACH_CLBK(mouse_motion);
  ATTACH_CLBK(mouse_button);
  ATTACH_CLBK(mouse_wheel);
  #undef ATTACH_CLBK

  /* Init the view transform from the app main view. */
  APP(get_main_view(ctxt->app, &main_view));
  APP(get_raw_view_transform(main_view, &f44));
  ctxt->view.transform = *f44;
  /* Reset the states and the input. */
  memset(&ctxt->states, 0, sizeof(ctxt->states));
  memset(&ctxt->inputs, 0, sizeof(ctxt->inputs));

  /* Retrieve the current input states. */
  WM(get_mouse_wheel(wm, &ctxt->states.mouse_wheel));
  WM(get_mouse_position
    (wm, ctxt->states.mouse_cursor + 0, ctxt->states.mouse_cursor + 1));

exit:
  return edit_err;
error:
  if(ctxt) {
    if(wm) {
      #define DETACH_CLBK(name) \
        do { \
          bool b = false; \
          WM(is_##name##_callback_attached(wm, name##_clbk, ctxt, &b)); \
          if(b && !is_##name##_clbk_attached) { \
            WM(detach_##name##_callback(wm, name##_clbk, ctxt)); \
          } \
        } while(0)
      DETACH_CLBK(key);
      DETACH_CLBK(mouse_motion);
      DETACH_CLBK(mouse_button);
      DETACH_CLBK(mouse_wheel);
      #undef DETACH_CLBK
    }
  }
  goto exit;
}

enum edit_error
edit_release_inputs(struct edit_context* ctxt)
{
  struct wm_device* wm = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;

  if(UNLIKELY(!ctxt)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }
  APP(get_window_manager_device(ctxt->app, &wm));

  #define DETACH_CLBK(name) \
    bool is_##name##_clbk_attached = false; \
    do { \
      WM(is_##name##_callback_attached \
        (wm, name##_clbk, ctxt, &is_##name##_clbk_attached)); \
      if(is_##name##_clbk_attached) { \
        wm_err = wm_detach_##name##_callback(wm, name##_clbk, ctxt); \
        if(wm_err != WM_NO_ERROR) { \
          edit_err = wm_to_edit_error(wm_err); \
          goto error; \
        } \
      } \
    } while(0)
  DETACH_CLBK(key);
  DETACH_CLBK(mouse_motion);
  DETACH_CLBK(mouse_button);
  DETACH_CLBK(mouse_wheel);
  #undef DETACH_CLBK
exit:
  return edit_err;
error:
  if(ctxt) {
    if(wm) {
      #define ATTACH_CLBK(name) \
        do { \
          bool b = false; \
          WM(is_##name##_callback_attached(wm, name##_clbk, ctxt, &b)); \
          if(!b && is_##name##_clbk_attached) { \
            WM(attach_##name##_callback(wm, name##_clbk, ctxt)); \
          } \
        } while(0)
      ATTACH_CLBK(key);
      ATTACH_CLBK(mouse_motion);
      ATTACH_CLBK(mouse_button);
      ATTACH_CLBK(mouse_wheel);
      #undef ATTACH_CLBK
    }
  }
  goto exit;
}

enum edit_error
edit_process_inputs(struct edit_context* ctxt)
{
  struct aosf33 pitch_roll_rot, yaw_rot, rot, inv_rot;
  vf4_t view_pos;


  if(UNLIKELY(!ctxt))
    return EDIT_INVALID_ARGUMENT;

  if(ctxt->inputs.updated == false)
    return EDIT_NO_ERROR;

  /* Locally translate the camera. */
  ctxt->view.transform.c3 = vf4_add
    (ctxt->view.transform.c3,
     vf4_set
      (ctxt->inputs.view_translation[0],
       ctxt->inputs.view_translation[1],
       ctxt->inputs.view_translation[2],
       0.f));

  /* Negate the rotation angle around the Y axis if the up vector of the view
   * transform point downward. */
  if(vf4_y(ctxt->view.transform.c1) < 0.f)
    ctxt->inputs.view_rotation[1] = -ctxt->inputs.view_rotation[1];
  aosf33_yaw_rotation(&yaw_rot, ctxt->inputs.view_rotation[1]);
  aosf33_rotation
    (&pitch_roll_rot,
     ctxt->inputs.view_rotation[0], 0.f, ctxt->inputs.view_rotation[2]);
  /* Retrieve the position of the camera in world space. */
  rot.c0 = ctxt->view.transform.c0;
  rot.c1 = ctxt->view.transform.c1;
  rot.c2 = ctxt->view.transform.c2;
  aosf33_inverse(&inv_rot, &rot);
  view_pos = aosf33_mulf3(&inv_rot, ctxt->view.transform.c3);
  /* Compute the new rotation matrix. */
  aosf33_mulf33(&rot, &pitch_roll_rot, &rot);
  aosf33_mulf33(&rot, &rot, &yaw_rot);
  ctxt->view.transform.c0 = rot.c0;
  ctxt->view.transform.c1 = rot.c1;
  ctxt->view.transform.c2 = rot.c2;
  /* Set the view position with respect to the new rotation matrix. */
  view_pos = aosf33_mulf3(&rot, view_pos);
  ctxt->view.transform.c3 = vf4_xyzd(view_pos, vf4_set1(1.f));

  /* Reset inputs. */
  memset(&ctxt->inputs, 0, sizeof(ctxt->inputs));

  return EDIT_NO_ERROR;
}

