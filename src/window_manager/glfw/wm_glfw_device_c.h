#ifndef WM_GLFW_DEVICE_C_H
#define WM_GLFW_DEVICE_C_H

#include "sys/ref_count.h"
#include "sys/sys.h"

struct mem_allocator;
struct sl_flat_set;
struct sl_hash_table;

typedef void (*wm_callback_t)(void);
#define WM_CALLBACK(func) ((wm_callback_t)func)

struct callback { 
  wm_callback_t func; 
  void* data;
};

struct wm_device {
  struct ref ref;
  struct mem_allocator* allocator;
  struct sl_flat_set* key_clbk_list;
  struct sl_flat_set* char_clbk_list;
  struct sl_flat_set* mouse_button_clbk_list;
  struct sl_flat_set* mouse_motion_clbk_list;
  struct sl_flat_set* mouse_wheel_clbk_list;
  struct sl_hash_table* glfw_to_wm_key_htbl;
};

/* Required by GLFW. */
LOCAL_SYM struct wm_device* g_device;

#endif
