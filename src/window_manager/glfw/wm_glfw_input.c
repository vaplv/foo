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

static FINLINE enum wm_mouse_button
glfw_to_wm_button(int button)
{
  enum wm_mouse_button wm_button = WM_MOUSE_BUTTON_0;
  switch(button) {
    case GLFW_MOUSE_BUTTON_1: wm_button = WM_MOUSE_BUTTON_0; break;
    case GLFW_MOUSE_BUTTON_2: wm_button = WM_MOUSE_BUTTON_1; break;
    case GLFW_MOUSE_BUTTON_3: wm_button = WM_MOUSE_BUTTON_2; break;
    case GLFW_MOUSE_BUTTON_4: wm_button = WM_MOUSE_BUTTON_3; break;
    case GLFW_MOUSE_BUTTON_5: wm_button = WM_MOUSE_BUTTON_4; break;
    case GLFW_MOUSE_BUTTON_6: wm_button = WM_MOUSE_BUTTON_5; break;
    case GLFW_MOUSE_BUTTON_7: wm_button = WM_MOUSE_BUTTON_6; break;
    case GLFW_MOUSE_BUTTON_8: wm_button = WM_MOUSE_BUTTON_7; break;
    default: assert(0); break;
  }
  return wm_button;
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

static enum wm_error
attach_clbk
  (struct sl_flat_set* list,
   wm_callback_t clbk,
   void* data,
   size_t* len)
{
  enum sl_error sl_err = SL_NO_ERROR;
  assert(list && clbk && data && len);

  SL(flat_set_buffer(list, len, NULL, NULL, NULL));
  sl_err = sl_flat_set_insert(list, (struct callback[]){{clbk, data}}, NULL);
  return sl_to_wm_error(sl_err);
}

static enum wm_error
detach_clbk
  (struct sl_flat_set* list,
   wm_callback_t clbk,
   void* data,
   size_t* len)
{
  enum sl_error sl_err = SL_NO_ERROR;
  assert(list && clbk && data && len);

  SL(flat_set_buffer(list, len, NULL, NULL, NULL));
  sl_err = sl_flat_set_erase(list, (struct callback[]){{clbk, data}}, NULL);
  return sl_to_wm_error(sl_err);
}

static bool
is_clbk_attached
  (struct sl_flat_set* list,
   wm_callback_t clbk,
   void* data)
{
  size_t len = 0, id = 0;
  assert(list && clbk && data);

  SL(flat_set_find(list, (struct callback[]){{clbk, data}}, &id));
  SL(flat_set_buffer(list, &len, NULL, NULL, NULL));
  return id != len;
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

static void
glfw_mouse_button_callback(int button, int action)
{
  struct callback* clbk_list = NULL;
  size_t nb_clbk = 0;
  size_t i = 0;
  const enum wm_mouse_button wm_button = glfw_to_wm_button(button);
  const enum wm_state wm_state = glfw_to_wm_state(action);
  assert(NULL != g_device);

  SL(flat_set_buffer
    (g_device->mouse_button_clbk_list, &nb_clbk, 0, 0, (void**)&clbk_list));
  for(i = 0; i < nb_clbk; ++i) {
    ((void (*)(enum wm_mouse_button, enum wm_state, void*))clbk_list[i].func)
      (wm_button, wm_state, clbk_list[i].data);
  }
}

static void
glfw_mouse_motion_callback(int x, int y)
{
  struct callback* clbk_list = NULL;
  size_t nb_clbk = 0;
  size_t i = 0;
  assert(NULL != g_device);

  SL(flat_set_buffer
    (g_device->mouse_motion_clbk_list, &nb_clbk, 0, 0, (void**)&clbk_list));
  for(i = 0; i < nb_clbk; ++i) {
    ((void (*)(int, int, void*))clbk_list[i].func)(x, y, clbk_list[i].data);
  }
}

static void
glfw_mouse_wheel_callback(int pos)
{
  struct callback* clbk_list = NULL;
  size_t nb_clbk = 0;
  size_t i = 0;
  assert(NULL != g_device);

  SL(flat_set_buffer
    (g_device->mouse_wheel_clbk_list, &nb_clbk, 0, 0, (void**)&clbk_list));
  for(i = 0; i < nb_clbk; ++i) {
    ((void (*)(int, void*))clbk_list[i].func)(pos, clbk_list[i].data);
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

  if(UNLIKELY(!device || !state)) {
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

  if(UNLIKELY(!device || !state)) {
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

  if(UNLIKELY(!device))
    return WM_INVALID_ARGUMENT;

  glfwGetMousePos(&x, &y);

  if(out_x)
    *out_x = x;
  if(out_y)
    *out_y = y;

  return WM_NO_ERROR;
}

EXPORT_SYM enum wm_error
wm_get_mouse_wheel(struct wm_device* device, int* pos)
{
  if(UNLIKELY(!device || !pos))
    return WM_INVALID_ARGUMENT;
  *pos = glfwGetMouseWheel();
  return WM_NO_ERROR;
}

#define DECLARE_ATTACH_CLBK_FUNC(name, glfw_name, ...) \
  EXPORT_SYM enum wm_error \
  CONCAT(CONCAT(wm_attach_, name), _callback) \
    (struct wm_device* device, \
     void (*func)(__VA_ARGS__), \
     void* data) \
  { \
    size_t len = 0; \
    enum wm_error wm_err = WM_NO_ERROR; \
    if(!device || !func) { \
      wm_err = WM_INVALID_ARGUMENT; \
      goto error; \
    } \
    wm_err = attach_clbk \
      (device->CONCAT(name, _clbk_list), WM_CALLBACK(func), data, &len); \
    if(wm_err != WM_NO_ERROR) \
      goto error; \
  /* If it is the first attached callback, setup the callback which dispatch \
   * the glfw key events to the attached callbacks. */ \
  if(len == 0) \
    CONCAT(CONCAT(glfwSet, glfw_name), Callback) \
      (CONCAT(CONCAT(glfw_, name), _callback)); \
  exit: \
    return wm_err; \
  error: \
    goto exit; \
  }
DECLARE_ATTACH_CLBK_FUNC(key, Key, enum wm_key, enum wm_state, void*)
DECLARE_ATTACH_CLBK_FUNC(char, Char, wchar_t, enum wm_state, void*)
DECLARE_ATTACH_CLBK_FUNC(mouse_motion, MousePos, int, int, void*)
DECLARE_ATTACH_CLBK_FUNC(mouse_wheel, MouseWheel, int, void*)
DECLARE_ATTACH_CLBK_FUNC
  (mouse_button, MouseButton, enum wm_mouse_button, enum wm_state, void*)
#undef DECLARE_ATTACH_CLBK_FUNC


#define DECLARE_DETACH_CLBK_FUNC(name, glfw_name, ...) \
  EXPORT_SYM enum wm_error \
  CONCAT(CONCAT(wm_detach_, name), _callback) \
    (struct wm_device* device, \
     void (*func)(__VA_ARGS__), \
     void* data) \
  { \
    size_t len = 0; \
    enum wm_error wm_err = WM_NO_ERROR; \
    if(!device || !func) { \
      wm_err = WM_INVALID_ARGUMENT; \
      goto error; \
    } \
    wm_err = detach_clbk \
      (device->CONCAT(name, _clbk_list), WM_CALLBACK(func), data, &len); \
    if(wm_err != WM_NO_ERROR) \
      goto error; \
    /* Detach the glfw callback if no more key callback is attached. */ \
    if(0 == len) \
      CONCAT(CONCAT(glfwSet, glfw_name), Callback)(NULL); \
  exit: \
    return wm_err; \
  error: \
    goto exit; \
  }
DECLARE_DETACH_CLBK_FUNC(key, Key, enum wm_key, enum wm_state, void*)
DECLARE_DETACH_CLBK_FUNC(char, Char, wchar_t, enum wm_state, void*)
DECLARE_DETACH_CLBK_FUNC(mouse_motion, MousePos, int, int, void*)
DECLARE_DETACH_CLBK_FUNC(mouse_wheel, MouseWheel, int, void*)
DECLARE_DETACH_CLBK_FUNC
  (mouse_button, MouseButton, enum wm_mouse_button, enum wm_state, void*)
#undef DECLARE_DETACH_CLBK_FUNC


#define DECLARE_IS_CLBK_ATTACHED_FUNC(name, ...) \
  EXPORT_SYM enum wm_error \
  CONCAT(CONCAT(wm_is_, name), _callback_attached) \
    (struct wm_device* device, \
     void (*func)(__VA_ARGS__), \
     void* data, \
     bool* is_attached) \
  { \
    if(!device || !func || !is_attached) \
      return WM_INVALID_ARGUMENT; \
    *is_attached = is_clbk_attached \
      (device->CONCAT(name, _clbk_list), WM_CALLBACK(func), data); \
    return WM_NO_ERROR; \
  }
DECLARE_IS_CLBK_ATTACHED_FUNC(key, enum wm_key, enum wm_state, void*)
DECLARE_IS_CLBK_ATTACHED_FUNC(char, wchar_t, enum wm_state, void*)
DECLARE_IS_CLBK_ATTACHED_FUNC(mouse_motion, int, int, void*)
DECLARE_IS_CLBK_ATTACHED_FUNC(mouse_wheel, int, void*)
DECLARE_IS_CLBK_ATTACHED_FUNC
  (mouse_button, enum wm_mouse_button, enum wm_state, void*)
#undef DECLARE_IS_CLBK_ATTACHED_FUNC

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

  #define CREATE_CLBK_FLAT_SET(out) \
    CALL(sl_create_flat_set \
      (sizeof(struct callback), \
       ALIGNOF(struct callback), \
       compare_callbacks, \
       device->allocator, \
       (out)))
  CREATE_CLBK_FLAT_SET(&device->key_clbk_list);
  CREATE_CLBK_FLAT_SET(&device->char_clbk_list);
  CREATE_CLBK_FLAT_SET(&device->mouse_button_clbk_list);
  CREATE_CLBK_FLAT_SET(&device->mouse_motion_clbk_list);
  CREATE_CLBK_FLAT_SET(&device->mouse_wheel_clbk_list);
  #undef CREATE_CLBK_FLAT_SET

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

  #define RELEASE_CLBK_FLAT_SET(clbk_flat_set) \
    do {  \
      if(clbk_flat_set) { \
        SL(free_flat_set(clbk_flat_set)); \
        (clbk_flat_set) = NULL; \
      } \
    } while(0)
  RELEASE_CLBK_FLAT_SET(device->key_clbk_list);
  RELEASE_CLBK_FLAT_SET(device->char_clbk_list);
  RELEASE_CLBK_FLAT_SET(device->mouse_button_clbk_list);
  RELEASE_CLBK_FLAT_SET(device->mouse_motion_clbk_list);
  RELEASE_CLBK_FLAT_SET(device->mouse_wheel_clbk_list);
  #undef RELEASE_CLBK_FLAT_SET

  if(device->glfw_to_wm_key_htbl) {
    SL(free_hash_table(device->glfw_to_wm_key_htbl));
    device->glfw_to_wm_key_htbl = NULL;
  }
  return WM_NO_ERROR;
}


