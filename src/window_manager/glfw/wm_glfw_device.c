#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "window_manager/glfw/wm_glfw_device_c.h"
#include "window_manager/wm_device.h"
#include <GL/glfw.h>
#include <stdlib.h>

EXPORT_SYM enum wm_error
wm_create_device
  (struct mem_allocator* specific_allocator,
   struct wm_device** out_device)
{
  struct mem_allocator* allocator = NULL;
  struct wm_device* device = NULL;
  enum wm_error err = WM_NO_ERROR;

  if(!out_device) {
    err = WM_INVALID_ARGUMENT;
    goto error;
  }
  allocator = specific_allocator ? specific_allocator : &mem_default_allocator;
  device = MEM_ALLOC_I(allocator, sizeof(struct wm_device));
  if(!device) {
    err = WM_MEMORY_ERROR;
    goto error;
  }
  device->allocator = allocator;
  if(glfwInit() == GL_FALSE) {
    err = WM_INTERNAL_ERROR;
    goto error;
  }
  glfwOpenWindowHint(GLFW_VERSION_MAJOR, 3);
  glfwOpenWindowHint(GLFW_VERSION_MINOR, 3);

exit:
  if(out_device)
    *out_device = device;
  return err;

error:
  if(device) {
    MEM_FREE_I(allocator, device);
    device = NULL;
  }
  goto exit;
}

EXPORT_SYM enum wm_error
wm_free_device(struct wm_device* device)
{
  struct mem_allocator* allocator = NULL;
  if(!device)
    return WM_INVALID_ARGUMENT;

  allocator = device->allocator;
  MEM_FREE_I(allocator, device);
  glfwTerminate();
  return WM_NO_ERROR;
}

EXPORT_SYM enum wm_error
wm_flush_events(struct wm_device* device)
{
  if(!device)
    return WM_INVALID_ARGUMENT;

  glfwPollEvents();
  return WM_NO_ERROR;
}

