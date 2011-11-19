#ifndef WM_GLFW_DEVICE_C_H
#define WM_GLFW_DEVICE_C_H

struct mem_allocator;

struct wm_device {
  struct mem_allocator* allocator;
};

#endif
