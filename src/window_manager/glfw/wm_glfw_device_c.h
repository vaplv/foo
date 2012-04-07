#ifndef WM_GLFW_DEVICE_C_H
#define WM_GLFW_DEVICE_C_H

#include "sys/ref_count.h"

struct mem_allocator;
struct sl_set;
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
  struct sl_set* key_clbk_list; /* Set of struct key_callback. */
  struct sl_set* char_clbk_list; /* Set of struct char_callback. */
  struct sl_hash_table* glfw_to_wm_key_htbl;
};

/* Required by GLFW. */
extern struct wm_device* g_device;

#endif
