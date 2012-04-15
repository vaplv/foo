#include "stdlib/sl.h"
#include "stdlib/sl_hash_table.h"
#include "stdlib/sl_flat_set.h"
#include "sys/sys.h"
#include "window_manager/glfw/wm_glfw_device_c.h"
#include "window_manager/glfw/wm_glfw_error.h"
#include "window_manager/glfw/wm_glfw_input.h"
#include "window_manager/wm.h"
#include "window_manager/wm_error.h"
#include "window_manager/wm_input.h"
#include <GL/glfw.h>
#include <assert.h>
#include <stdbool.h>

static const int
wm_to_glfw_mouse_button_lut[] = {
  [WM_MOUSE_BUTTON_0] = GLFW_MOUSE_BUTTON_1,
  [WM_MOUSE_BUTTON_1] = GLFW_MOUSE_BUTTON_2,
  [WM_MOUSE_BUTTON_2] = GLFW_MOUSE_BUTTON_3,
  [WM_MOUSE_BUTTON_3] = GLFW_MOUSE_BUTTON_4,
  [WM_MOUSE_BUTTON_4] = GLFW_MOUSE_BUTTON_5,
  [WM_MOUSE_BUTTON_5] = GLFW_MOUSE_BUTTON_6,
  [WM_MOUSE_BUTTON_6] = GLFW_MOUSE_BUTTON_7,
  [WM_MOUSE_BUTTON_7] = GLFW_MOUSE_BUTTON_8
};

static const int
wm_to_glfw_key_lut[] = {
  [WM_KEY_A] = 'A',
  [WM_KEY_B] = 'B',
  [WM_KEY_C] = 'C',
  [WM_KEY_D] = 'D',
  [WM_KEY_E] = 'E',
  [WM_KEY_F] = 'F',
  [WM_KEY_G] = 'G',
  [WM_KEY_H] = 'H',
  [WM_KEY_I] = 'I',
  [WM_KEY_J] = 'J',
  [WM_KEY_K] = 'K',
  [WM_KEY_L] = 'L',
  [WM_KEY_M] = 'M',
  [WM_KEY_N] = 'N',
  [WM_KEY_O] = 'O',
  [WM_KEY_P] = 'P',
  [WM_KEY_Q] = 'Q',
  [WM_KEY_R] = 'R',
  [WM_KEY_S] = 'S',
  [WM_KEY_T] = 'T',
  [WM_KEY_U] = 'U',
  [WM_KEY_V] = 'V',
  [WM_KEY_W] = 'W',
  [WM_KEY_X] = 'X',
  [WM_KEY_Y] = 'Y',
  [WM_KEY_Z] = 'Z',
  [WM_KEY_0] = '0',
  [WM_KEY_1] = '1',
  [WM_KEY_2] = '2',
  [WM_KEY_3] = '3',
  [WM_KEY_4] = '4',
  [WM_KEY_5] = '5',
  [WM_KEY_6] = '6',
  [WM_KEY_7] = '7',
  [WM_KEY_8] = '8',
  [WM_KEY_9] = '9',
  [WM_KEY_DOT] = '.',
  [WM_KEY_SQUARE] = 0xB2,
  [WM_KEY_SPACE] = GLFW_KEY_SPACE,
  [WM_KEY_ESC] = GLFW_KEY_ESC,
  [WM_KEY_F1] = GLFW_KEY_F1,
  [WM_KEY_F2] = GLFW_KEY_F2,
  [WM_KEY_F3] = GLFW_KEY_F3,
  [WM_KEY_F4] = GLFW_KEY_F4,
  [WM_KEY_F5] = GLFW_KEY_F5,
  [WM_KEY_F6] = GLFW_KEY_F6,
  [WM_KEY_F7] = GLFW_KEY_F7,
  [WM_KEY_F8] = GLFW_KEY_F8,
  [WM_KEY_F9] = GLFW_KEY_F9,
  [WM_KEY_F10] = GLFW_KEY_F10,
  [WM_KEY_F11] = GLFW_KEY_F11,
  [WM_KEY_F12] = GLFW_KEY_F12,
  [WM_KEY_UP] = GLFW_KEY_UP,
  [WM_KEY_DOWN] = GLFW_KEY_DOWN,
  [WM_KEY_LEFT] = GLFW_KEY_LEFT,
  [WM_KEY_RIGHT] = GLFW_KEY_RIGHT,
  [WM_KEY_LSHIFT] = GLFW_KEY_LSHIFT,
  [WM_KEY_RSHIFT] = GLFW_KEY_RSHIFT,
  [WM_KEY_LCTRL] = GLFW_KEY_LCTRL,
  [WM_KEY_RCTRL] = GLFW_KEY_RCTRL,
  [WM_KEY_LALT] = GLFW_KEY_LALT,
  [WM_KEY_RALT] = GLFW_KEY_RALT,
  [WM_KEY_TAB] = GLFW_KEY_TAB,
  [WM_KEY_ENTER] = GLFW_KEY_ENTER,
  [WM_KEY_BACKSPACE] = GLFW_KEY_BACKSPACE,
  [WM_KEY_INSERT] = GLFW_KEY_INSERT,
  [WM_KEY_DEL] = GLFW_KEY_DEL,
  [WM_KEY_PAGEUP] = GLFW_KEY_PAGEUP,
  [WM_KEY_PAGEDOWN] = GLFW_KEY_PAGEDOWN,
  [WM_KEY_HOME] = GLFW_KEY_HOME,
  [WM_KEY_END] = GLFW_KEY_END,
  [WM_KEY_KP_0] = GLFW_KEY_KP_0,
  [WM_KEY_KP_1] = GLFW_KEY_KP_1,
  [WM_KEY_KP_2] = GLFW_KEY_KP_2,
  [WM_KEY_KP_3] = GLFW_KEY_KP_3,
  [WM_KEY_KP_4] = GLFW_KEY_KP_4,
  [WM_KEY_KP_5] = GLFW_KEY_KP_5,
  [WM_KEY_KP_6] = GLFW_KEY_KP_6,
  [WM_KEY_KP_7] = GLFW_KEY_KP_7,
  [WM_KEY_KP_8] = GLFW_KEY_KP_8,
  [WM_KEY_KP_9] = GLFW_KEY_KP_9,
  [WM_KEY_KP_DIV] = GLFW_KEY_KP_DIVIDE,
  [WM_KEY_KP_MUL] = GLFW_KEY_KP_MULTIPLY,
  [WM_KEY_KP_SUB] = GLFW_KEY_KP_SUBTRACT,
  [WM_KEY_KP_ADD] = GLFW_KEY_KP_ADD,
  [WM_KEY_KP_DOT] = GLFW_KEY_KP_DECIMAL,
  [WM_KEY_KP_EQUAL] = GLFW_KEY_KP_EQUAL,
  [WM_KEY_KP_ENTER] = GLFW_KEY_KP_ENTER,
  [WM_KEY_KP_NUM_LOCK] = GLFW_KEY_UNKNOWN, /* unsupported */
  [WM_KEY_CAPS_LOCK] = GLFW_KEY_UNKNOWN, /* unsupported */
  [WM_KEY_SCROLL_LOCK] = GLFW_KEY_UNKNOWN, /* unsupported */
  [WM_KEY_PAUSE] = GLFW_KEY_UNKNOWN, /* unsupported */
  [WM_KEY_MENU] = GLFW_KEY_UNKNOWN, /* unsupported */
  [WM_KEY_UNKNOWN] = GLFW_KEY_UNKNOWN /* unsupported */
};

/*******************************************************************************
 *
 * Set and hash table helper functions.
 *
 ******************************************************************************/
static int
compare_callbacks(const void* a, const void* b)
{
  struct callback* cbk0 = (struct callback*)a;
  struct callback* cbk1 = (struct callback*)b;
  const uintptr_t p0[2] = {(uintptr_t)cbk0->func, (uintptr_t)cbk0->data};
  const uintptr_t p1[2] = {(uintptr_t)cbk1->func, (uintptr_t)cbk1->data};
  const int inf = (p0[0] < p1[0]) | ((p0[0] == p1[0]) & (p0[1] < p1[1]));
  const int sup = (p0[0] > p1[0]) | ((p0[0] == p1[0]) & (p0[1] > p1[1]));
  return -(inf) | (sup);
}

static size_t
hash(const void* key) 
{
  return sl_hash(key, sizeof(int));
}

static bool
eq_key(const void* a, const void* b)
{
  return (*(int*)a) == (*(int*)b);
}

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static FINLINE int
wm_to_glfw_key(enum wm_key key)
{
  assert(key < sizeof(wm_to_glfw_key_lut));
  return wm_to_glfw_key_lut[key];
}

static FINLINE enum wm_key
glfw_to_wm_key(struct wm_device* device, int key)
{
  enum wm_key* k = NULL;
  assert(device);

  SL(hash_table_find
    (device->glfw_to_wm_key_htbl, (int[]){key}, (void**)&k));
  if(k) {
    return *k;
  } else {
    return WM_KEY_UNKNOWN;
  }
}

static FINLINE int
wm_to_glfw_mouse_button(enum wm_mouse_button button)
{
  assert(button < sizeof(wm_to_glfw_mouse_button_lut));
  return wm_to_glfw_mouse_button_lut[button];
}

static FINLINE enum wm_state
glfw_to_wm_state(int state)
{
  assert(state == GLFW_PRESS || state == GLFW_RELEASE);
  if(state == GLFW_PRESS)
    return WM_PRESS;
  else
    return WM_RELEASE;
}

static void
glfw_key_callback(int key, int action)
{
  struct callback* clbk_list = NULL;
  size_t nb_clbk = 0;
  size_t i = 0;
  const enum wm_key wm_key = glfw_to_wm_key(g_device, key);
  const enum wm_state wm_state = glfw_to_wm_state(action);
  assert(NULL != g_device);

  SL(flat_set_buffer
    (g_device->key_clbk_list, &nb_clbk, 0, 0, (void**)&clbk_list));
  for(i = 0; i < nb_clbk; ++i) {
    ((void (*)(enum wm_key, enum wm_state, void*))clbk_list[i].func)
      (wm_key, wm_state, clbk_list[i].data);
  }
}

static void
glfw_char_callback(int character, int action)
{
  struct callback* clbk_list = NULL;
  size_t nb_clbk = 0;
  size_t i = 0;
  wchar_t ch = (wchar_t)character;
  const enum wm_state wm_state = glfw_to_wm_state(action);
  assert(NULL != g_device);

  SL(flat_set_buffer
    (g_device->char_clbk_list, &nb_clbk, 0, 0, (void**)&clbk_list));
  for(i = 0; i < nb_clbk; ++i) {
    ((void (*)(wchar_t, enum wm_state, void*))clbk_list[i].func)
      (ch, wm_state, clbk_list[i].data);
  }
}

/*******************************************************************************
 *
 * Input implementation.
 *
 ******************************************************************************/
EXPORT_SYM enum wm_error
wm_get_key_state
  (struct wm_device* device,
   enum wm_key key,
   enum wm_state* state)
{
  enum wm_error wm_err = WM_NO_ERROR;
  int glfw_key = GLFW_KEY_UNKNOWN;

  if(!device || !state) {
    wm_err = WM_INVALID_ARGUMENT;
    goto error;
  }

  glfw_key = wm_to_glfw_key(key);
  if(glfw_key == GLFW_KEY_UNKNOWN) {
    *state = WM_STATE_UNKNOWN;
  } else {
    const int glfw_state = glfwGetKey(glfw_key);
    *state = glfw_to_wm_state(glfw_state);
  }

exit:
  return wm_err;

error:
  goto exit;
}

EXPORT_SYM enum wm_error
wm_get_mouse_button_state
  (struct wm_device* device,
   enum wm_mouse_button button,
   enum wm_state* state)
{
  enum wm_error wm_err = WM_NO_ERROR;
  int glfw_button = GLFW_MOUSE_BUTTON_1;
  int glfw_state = GLFW_RELEASE;

  if(!device || !state) {
    wm_err = WM_INVALID_ARGUMENT;
    goto error;
  }

  glfw_button = wm_to_glfw_mouse_button(button);
  glfw_state = glfwGetMouseButton(glfw_button);
  *state = glfw_to_wm_state(glfw_state);

exit:
  return wm_err;

error:
  goto exit;
}

EXPORT_SYM enum wm_error
wm_get_mouse_position
  (struct wm_device* device,
   int* out_x,
   int* out_y)
{
  int x = 0;
  int y = 0;

  if(!device)
    return WM_INVALID_ARGUMENT;

  glfwGetMousePos(&x, &y);

  if(out_x)
    *out_x = x;
  if(out_y)
    *out_y = y;

  return WM_NO_ERROR;
}

EXPORT_SYM enum wm_error
wm_attach_key_callback
  (struct wm_device* device,
   void (*func)(enum wm_key, enum wm_state, void*),
   void* data)
{
  size_t len = 0;
  enum sl_error sl_err = SL_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;

  if(!device || !func) {
    wm_err = WM_INVALID_ARGUMENT;
    goto error;
  }
  /* If it is the first attached callback, setup the callback which dispatch
   * the glfw key events to the attached callbacks. */
  SL(flat_set_buffer(device->key_clbk_list, &len, NULL, NULL, NULL));
  if(0 == len)
    glfwSetKeyCallback(glfw_key_callback);

  sl_err = sl_flat_set_insert
    (device->key_clbk_list, 
     (struct callback[]){{WM_CALLBACK(func), data}}, 
     NULL);
  if(sl_err != SL_NO_ERROR) {
    wm_err = sl_to_wm_error(sl_err);
    goto error;
  }

exit:
  return wm_err;
error:
  goto exit;
}

EXPORT_SYM enum wm_error
wm_detach_key_callback
  (struct wm_device* device,
   void (*func)(enum wm_key, enum wm_state, void*),
   void* data)
{
  size_t len = 0;
  enum sl_error sl_err = SL_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;

  if(!device || !func) {
    wm_err = WM_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_flat_set_erase
    (device->key_clbk_list, 
     (struct callback[]){{WM_CALLBACK(func), data}},
     NULL);
  if(sl_err != SL_NO_ERROR) {
    wm_err = sl_to_wm_error(sl_err);
    goto error;
  }
  /* Detach the glfw callback if no more key callback is attached. */
  SL(flat_set_buffer(device->key_clbk_list, &len, NULL, NULL, NULL));
  if(0 == len)
    glfwSetKeyCallback(NULL);

exit:
  return wm_err;
error:
  goto exit;
}

EXPORT_SYM enum wm_error
wm_is_key_callback_attached
  (struct wm_device* device,
   void (*func)(enum wm_key, enum wm_state, void*),
   void* data,
   bool* is_attached)
{
  size_t len = 0;
  size_t id = 0;
  enum wm_error wm_err = WM_NO_ERROR;

  if(!device || !func || !is_attached) {
    wm_err = WM_INVALID_ARGUMENT;
    goto error;
  }
  SL(flat_set_find
    (device->key_clbk_list, 
     (struct callback[]){{WM_CALLBACK(func), data}},
     &id));
  SL(flat_set_buffer(device->key_clbk_list, &len, NULL, NULL, NULL));
  *is_attached = id != len;
exit:
  return wm_err;
error:
  goto exit;
}

EXPORT_SYM enum wm_error
wm_attach_char_callback
  (struct wm_device* device,
   void (*func)(wchar_t, enum wm_state, void*),
   void* data)
{
  size_t len = 0;
  enum sl_error sl_err = SL_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;

  if(!device || !func) {
    wm_err = WM_INVALID_ARGUMENT;
    goto error;
  }
  /* If it is the first attached callback, setup the callback which dispatch
   * the glfw char events to the attached callbacks. */
  SL(flat_set_buffer(device->char_clbk_list, &len, NULL, NULL, NULL));
  if(0 == len)
    glfwSetCharCallback(glfw_char_callback);

  sl_err = sl_flat_set_insert
    (device->char_clbk_list, 
     (struct callback[]){{WM_CALLBACK(func), data}}, 
     NULL);
  if(sl_err != SL_NO_ERROR) {
    wm_err = sl_to_wm_error(sl_err);
    goto error;
  }

exit:
  return wm_err;
error:
  goto exit;

}

EXPORT_SYM enum wm_error
wm_detach_char_callback
  (struct wm_device* device,
   void (*func)(wchar_t, enum wm_state, void*),
   void* data)
{
  size_t len = 0;
  enum sl_error sl_err = SL_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;

  if(!device || !func) {
    wm_err = WM_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_flat_set_erase
    (device->char_clbk_list, 
     (struct callback[]){{WM_CALLBACK(func), 
     data}},
     NULL);
  if(sl_err != SL_NO_ERROR) {
    wm_err = sl_to_wm_error(sl_err);
    goto error;
  }
  /* Detach the glfw callback if no more char callback is attached. */
  SL(flat_set_buffer(device->char_clbk_list, &len, NULL, NULL, NULL));
  if(0 == len)
    glfwSetCharCallback(NULL);

exit:
  return wm_err;
error:
  goto exit;
}

EXPORT_SYM enum wm_error
wm_is_char_callback_attached
  (struct wm_device* device,
   void (*func)(wchar_t, enum wm_state, void*),
   void* data,
   bool* is_attached)
{
  size_t len = 0;
  size_t id = 0;
  enum wm_error wm_err = WM_NO_ERROR;

  if(!device || !func || !is_attached) {
    wm_err = WM_INVALID_ARGUMENT;
    goto error;
  }
  SL(flat_set_find
    (device->char_clbk_list, 
     (struct callback[]){{WM_CALLBACK(func), data}},
     &id));
  SL(flat_set_buffer(device->char_clbk_list, &len, NULL, NULL, NULL));
  *is_attached = id != len;
exit:
  return wm_err;
error:
  goto exit;
}

/*******************************************************************************
 *
 * Private input functions.
 *
 ******************************************************************************/
enum wm_error
wm_init_inputs(struct wm_device* device)
{
  enum sl_error sl_err = SL_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;
  size_t len = 0;
  size_t i = 0;

  if(!device) {
    wm_err = WM_INVALID_ARGUMENT;
    goto error;
  }

  #define CALL(func) \
    do { \
      if(SL_NO_ERROR != (sl_err = func)) { \
        wm_err = sl_to_wm_error(sl_err); \
        goto error; \
      } \
    } while(0)

  CALL(sl_create_flat_set
    (sizeof(struct callback),
     ALIGNOF(struct callback),
     compare_callbacks,
     device->allocator,
     &device->key_clbk_list));
  CALL(sl_create_flat_set
    (sizeof(struct callback),
     ALIGNOF(struct callback),
     compare_callbacks,
     device->allocator,
     &device->char_clbk_list));
  CALL(sl_create_hash_table
    (sizeof(int),
     ALIGNOF(int),
     sizeof(enum wm_key),
     ALIGNOF(enum wm_key),
     hash,
     eq_key,
     device->allocator,
     &device->glfw_to_wm_key_htbl));
  /* Fill the hash table. */
  len = sizeof(wm_to_glfw_key_lut) / sizeof(int);
  for(i = 0; i < len; ++i) {
    CALL(sl_hash_table_insert
      (device->glfw_to_wm_key_htbl, 
       (int[]){wm_to_glfw_key_lut[i]},
       (enum wm_key[]){(enum wm_key)i}));
  }
  #undef CALL
exit:
  return wm_err;
error:
  WM(shutdown_inputs(device));
  goto exit;
}

extern enum wm_error
wm_shutdown_inputs(struct wm_device* device)
{
  if(!device)
    return WM_INVALID_ARGUMENT;

  if(device->key_clbk_list) {
    SL(free_flat_set(device->key_clbk_list));
    device->key_clbk_list = NULL;
  }
  if(device->char_clbk_list) {
    SL(free_flat_set(device->char_clbk_list));
    device->char_clbk_list = NULL;
  }
  if(device->glfw_to_wm_key_htbl) {
    SL(free_hash_table(device->glfw_to_wm_key_htbl));
    device->glfw_to_wm_key_htbl = NULL;
  }
  return WM_NO_ERROR;
}


