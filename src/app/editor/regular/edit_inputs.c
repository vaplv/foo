#include "app/core/app.h"
#include "app/editor/regular/edit_context_c.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_inputs.h"
#include "window_manager/wm.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_input.h"

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
      ctxt->states.move_flag = EDIT_MOVE_ROTATE;
      break;
    case WM_KEY_T:
      ctxt->states.move_flag = EDIT_MOVE_TRANSLATE;
      break;
    case WM_KEY_E:
      ctxt->states.move_flag = EDIT_MOVE_SCALE;
      break;
    case WM_KEY_P:
      ctxt->states.move_flag = EDIT_MOVE_NONE;
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
  assert(ctxt);

  printf("%d %d\n", x, y);
}

static void
mouse_button_clbk(enum wm_mouse_button button, enum wm_state state, void* data)
{
  struct edit_context* ctxt = data;
  assert(ctxt);
  printf("%d %d\n", button, state);
}

/*******************************************************************************
 *
 * Input functions
 *
 ******************************************************************************/
enum edit_error
edit_init_inputs(struct edit_context* ctxt)
{
  struct wm_device* wm = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;

  if(!ctxt) {
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
  #undef ATTACH_CLBK

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

  if(!ctxt) {
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
      #undef ATTACH_CLBK
    }
  }
  goto exit;
}

