#include "stdlib/sl.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "window_manager/glfw/wm_glfw_device_c.h"
#include "window_manager/glfw/wm_glfw_error.h"
#include "window_manager/glfw/wm_glfw_input.h"
#include "window_manager/wm.h"
#include "window_manager/wm_device.h"
#include <GL/glfw.h>
#include <stdlib.h>

struct wm_device* g_device = NULL;

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_device(struct ref* ref)
{
  struct wm_device* device = NULL;
  assert(ref);

  device = CONTAINER_OF(ref, struct wm_device, ref);

  WM(shutdown_inputs(device));
  if(g_device) {
    assert(device == g_device);
    g_device = NULL;
    glfwSetKeyCallback(NULL);
    glfwSetCharCallback(NULL);
    glfwTerminate();
  }
  MEM_FREE(device->allocator, device);
}

/*******************************************************************************
 *
 * Device functions.
 *
 ******************************************************************************/
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
  if(NULL != g_device) {
    err = WM_INVALID_CALL;
    goto error;
  }
  allocator = specific_allocator ? specific_allocator : &mem_default_allocator;
  device = MEM_ALLOC(allocator, sizeof(struct wm_device));
  if(!device) {
    err = WM_MEMORY_ERROR;
    goto error;
  }
  device->allocator = allocator;
  ref_init(&device->ref);

  if(GL_FALSE == glfwInit()) {
    err = WM_INTERNAL_ERROR;
    goto error;
  }
  glfwOpenWindowHint(GLFW_VERSION_MAJOR, 3);
  glfwOpenWindowHint(GLFW_VERSION_MINOR, 3);
  glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);

  err = wm_init_inputs(device);
  if(err != WM_NO_ERROR)
    goto error;

  g_device = device;

exit:
  if(out_device)
    *out_device = device;
  return err;

error:
  if(device) {
    WM(device_ref_put(device));
    device = NULL;
  }
  goto exit;
}

EXPORT_SYM enum wm_error
wm_device_ref_get(struct wm_device* device)
{
  if(!device)
    return WM_INVALID_ARGUMENT;
  ref_get(&device->ref);
  return WM_NO_ERROR;
}

EXPORT_SYM enum wm_error
wm_device_ref_put(struct wm_device* device)
{
  if(!device)
    return WM_INVALID_ARGUMENT;
  ref_put(&device->ref, release_device);
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

