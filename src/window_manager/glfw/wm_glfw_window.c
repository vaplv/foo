#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include "window_manager/glfw/wm_glfw_device_c.h"
#include "window_manager/wm.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"
#include <GL/glfw.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>

struct wm_window {
  struct ref ref;
  struct wm_device* device;
  bool fullscreen;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_window(struct ref* ref)
{
  struct wm_device* dev = NULL;
  struct wm_window* win = NULL;
  assert(ref);

  win = CONTAINER_OF(ref, struct wm_window, ref);
  dev = win->device;

  glfwEnable(GLFW_AUTO_POLL_EVENTS);
  glfwDisable(GLFW_KEY_REPEAT);
  glfwCloseWindow();
  MEM_FREE(dev->allocator, win);
  WM(device_ref_put(dev));
}

/*******************************************************************************
 *
 * Window functions.
 *
 ******************************************************************************/
EXPORT_SYM enum wm_error
wm_create_window
  (struct wm_device* device,
   const struct wm_window_desc* desc,
   struct wm_window** out_win)
{
  struct wm_window* win = NULL;
  enum wm_error wm_err = WM_NO_ERROR;
  int width = 0;
  int height = 0;
  int mode = 0; 
  
  if(!device 
  || !out_win 
  || !desc
  || desc->width > INT_MAX
  || desc->height > INT_MAX) {
    wm_err = WM_INVALID_ARGUMENT;
    goto error;
  }

  win = MEM_CALLOC(device->allocator, 1, sizeof(struct wm_window));
  if(!win) {
    wm_err = WM_MEMORY_ERROR;
    goto error;
  }
  WM(device_ref_get(device));
  win->device = device;
  win->fullscreen = desc->fullscreen;
  ref_init(&win->ref);

  width = (int)desc->width;
  height = (int)desc->height;
  mode = desc->fullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW;
  if(glfwOpenWindow(width, height, 8, 8, 8, 8, 24, 8, mode) == GL_FALSE) {
    wm_err = WM_INTERNAL_ERROR;
    goto error;
  }
  glfwEnable(GLFW_KEY_REPEAT);
  glfwDisable(GLFW_AUTO_POLL_EVENTS);

exit:
  if(out_win)
    *out_win = win;
  return wm_err;
error:
  if(win) {
    WM(window_ref_put(win));
    win = NULL;
  }
  goto exit;
}

EXPORT_SYM enum wm_error
wm_window_ref_get(struct wm_window* win)
{
  if(!win)
    return WM_INVALID_ARGUMENT;
  ref_get(&win->ref);
  return WM_NO_ERROR;
}

EXPORT_SYM enum wm_error
wm_window_ref_put(struct wm_window* win)
{
  if(!win)
    return WM_INVALID_ARGUMENT;
  ref_put(&win->ref, release_window);
  return WM_NO_ERROR;
}

EXPORT_SYM enum wm_error
wm_swap(struct wm_window* win)
{
  if(!win)
    return WM_INVALID_ARGUMENT;
  glfwSwapBuffers();
  return WM_NO_ERROR;
}

EXPORT_SYM enum wm_error
wm_get_window_desc
  (struct wm_window* win,
   struct wm_window_desc* desc)
{
  int width = 0;
  int height = 0;

  if(!win || !desc)
    return WM_INVALID_ARGUMENT;

  glfwGetWindowSize(&width, &height);
  assert(width >= 0 && height >= 0);

  desc->width = (unsigned int)width;
  desc->height = (unsigned int)height;
  desc->fullscreen = win->fullscreen;
  return WM_NO_ERROR;
}

