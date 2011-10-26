#include "sys/sys.h"
#include "window_manager/wm_device.h"
#include <GL/glfw.h>
#include <stdlib.h>

struct wm_device {
  char dummy;
};

EXPORT_SYM enum wm_error
wm_create_device(struct wm_device** out_device)
{
  struct wm_device* device = NULL;
  enum wm_error err = WM_NO_ERROR;

  if(!out_device) {
    err = WM_INVALID_ARGUMENT;
    goto error;
  }
  device = malloc(sizeof(struct wm_device));
  if(!device) {
    err = WM_MEMORY_ERROR;
    goto error;
  }
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
  free(device);
  goto exit;
}

EXPORT_SYM enum wm_error
wm_free_device(struct wm_device* device)
{
  if(!device)
    return WM_INVALID_ARGUMENT;

  free(device);
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

