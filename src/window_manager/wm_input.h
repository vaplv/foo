#ifndef WM_INPUT_H
#define WM_INPUT_H

#include "window_manager/wm.h"
#include "window_manager/wm_error.h"
#include <stdbool.h>
#include <wchar.h>

enum wm_state {
  WM_PRESS,
  WM_RELEASE,
  WM_STATE_UNKNOWN
};

enum wm_mouse_button {
  WM_MOUSE_BUTTON_0, /* Left. */
  WM_MOUSE_BUTTON_1, /* Right. */
  WM_MOUSE_BUTTON_2, /* Middle. */
  WM_MOUSE_BUTTON_3,
  WM_MOUSE_BUTTON_4,
  WM_MOUSE_BUTTON_5,
  WM_MOUSE_BUTTON_6,
  WM_MOUSE_BUTTON_7
};

enum wm_key {
  WM_KEY_A,
  WM_KEY_B,
  WM_KEY_C,
  WM_KEY_D,
  WM_KEY_E,
  WM_KEY_F,
  WM_KEY_G,
  WM_KEY_H,
  WM_KEY_I,
  WM_KEY_J,
  WM_KEY_K,
  WM_KEY_L,
  WM_KEY_M,
  WM_KEY_N,
  WM_KEY_O,
  WM_KEY_P,
  WM_KEY_Q,
  WM_KEY_R,
  WM_KEY_S,
  WM_KEY_T,
  WM_KEY_U,
  WM_KEY_V,
  WM_KEY_W,
  WM_KEY_X,
  WM_KEY_Y,
  WM_KEY_Z,
  WM_KEY_0,
  WM_KEY_1,
  WM_KEY_2,
  WM_KEY_3,
  WM_KEY_4,
  WM_KEY_5,
  WM_KEY_6,
  WM_KEY_7,
  WM_KEY_8,
  WM_KEY_9,
  WM_KEY_DOT,
  WM_KEY_SQUARE,
  WM_KEY_SPACE,
  WM_KEY_ESC,
  WM_KEY_F1,
  WM_KEY_F2,
  WM_KEY_F3,
  WM_KEY_F4,
  WM_KEY_F5,
  WM_KEY_F6,
  WM_KEY_F7,
  WM_KEY_F8,
  WM_KEY_F9,
  WM_KEY_F10,
  WM_KEY_F11,
  WM_KEY_F12,
  WM_KEY_UP,
  WM_KEY_DOWN,
  WM_KEY_LEFT,
  WM_KEY_RIGHT,
  WM_KEY_LSHIFT,
  WM_KEY_RSHIFT,
  WM_KEY_LCTRL,
  WM_KEY_RCTRL,
  WM_KEY_LALT,
  WM_KEY_RALT,
  WM_KEY_TAB,
  WM_KEY_ENTER,
  WM_KEY_BACKSPACE,
  WM_KEY_INSERT,
  WM_KEY_DEL,
  WM_KEY_PAGEUP,
  WM_KEY_PAGEDOWN,
  WM_KEY_HOME,
  WM_KEY_END,
  WM_KEY_KP_0,
  WM_KEY_KP_1,
  WM_KEY_KP_2,
  WM_KEY_KP_3,
  WM_KEY_KP_4,
  WM_KEY_KP_5,
  WM_KEY_KP_6,
  WM_KEY_KP_7,
  WM_KEY_KP_8,
  WM_KEY_KP_9,
  WM_KEY_KP_DIV,
  WM_KEY_KP_MUL,
  WM_KEY_KP_SUB,
  WM_KEY_KP_ADD,
  WM_KEY_KP_DOT,
  WM_KEY_KP_EQUAL,
  WM_KEY_KP_ENTER,
  WM_KEY_KP_NUM_LOCK,
  WM_KEY_CAPS_LOCK,
  WM_KEY_SCROLL_LOCK,
  WM_KEY_PAUSE,
  WM_KEY_MENU,
  WM_KEY_UNKNOWN
};

struct wm_device;

WM_API enum wm_error
wm_get_key_state
  (struct wm_device* device,
   enum wm_key key,
   enum wm_state* state);

WM_API enum wm_error
wm_get_mouse_button_state
  (struct wm_device* device,
   enum wm_mouse_button button,
   enum wm_state* state);

/* (0, 0) is the upper left corner */
WM_API enum wm_error
wm_get_mouse_position
  (struct wm_device* device,
   int* x, /* May be NULL. */
   int* y); /* May be NULL. */

WM_API enum wm_error
wm_get_mouse_wheel
  (struct wm_device* device,
   int* pos);

WM_API enum wm_error
wm_attach_key_callback
  (struct wm_device* device,
   void (*func)(enum wm_key, enum wm_state, void* data),
   void* data); /* May be NULL. */

WM_API enum wm_error
wm_detach_key_callback
  (struct wm_device* device,
   void (*func)(enum wm_key, enum wm_state, void* data),
   void* data); /* May be NULL. */

WM_API enum wm_error
wm_is_key_callback_attached
  (struct wm_device* device,
   void (*func)(enum wm_key, enum wm_state, void* data),
   void* data, /* May be NULL. */
   bool* is_attached);

WM_API enum wm_error
wm_attach_char_callback
  (struct wm_device* device,
   void (*func)(wchar_t, enum wm_state, void* data),
   void* data); /* May be NULL. */

WM_API enum wm_error
wm_detach_char_callback
  (struct wm_device* device,
   void (*func)(wchar_t, enum wm_state, void* data),
   void* data); /* May be NULL. */

WM_API enum wm_error
wm_is_char_callback_attached
  (struct wm_device* device,
   void (*func)(wchar_t, enum wm_state, void* data),
   void* data, /* May be NULL. */
   bool* is_attached);

WM_API enum wm_error
wm_attach_mouse_button_callback
  (struct wm_device* device,
   void (*func)(enum wm_mouse_button, enum wm_state, void* data),
   void* data); /* May be NULL. */

WM_API enum wm_error
wm_detach_mouse_button_callback
  (struct wm_device* device,
   void (*func)(enum wm_mouse_button, enum wm_state, void* data),
   void* data); /* May be NULL. */

WM_API enum wm_error
wm_is_mouse_button_callback_attached
  (struct wm_device* device,
   void (*func)(enum wm_mouse_button, enum wm_state, void* data),
   void* data, /* May be NULL. */
   bool* is_attached);

WM_API enum wm_error
wm_attach_mouse_motion_callback
  (struct wm_device* device,
   void (*func)(int x, int y, void* data),
   void* data); /* May be NULL. */

WM_API enum wm_error
wm_detach_mouse_motion_callback
  (struct wm_device* device,
   void (*func)(int x, int y, void* data),
   void* data); /* May be NULL. */

WM_API enum wm_error
wm_is_mouse_motion_callback_attached
  (struct wm_device* device,
   void (*func)(int x, int y, void* data),
   void* data, /* May be NULL. */
   bool* is_attached);

WM_API enum wm_error
wm_attach_mouse_wheel_callback
  (struct wm_device* device,
   void (*func)(int pos, void* data),
   void* data); /* May be NULL. */

WM_API enum wm_error
wm_detach_mouse_wheel_callback
  (struct wm_device* device,
   void (*func)(int pos, void* data),
   void* data); /* May be NULL. */

WM_API enum wm_error
wm_is_mouse_wheel_callback_attached
  (struct wm_device* device,
   void (*func)(int pos, void* data),
   void* data, /* May be NULL. */
   bool* is_attached);

#endif /* WM_INPUT_H */

