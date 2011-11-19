#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "window_manager/glfw/wm_glfw_device_c.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"
#include <GL/glfw.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>

struct wm_window {
  bool fullscreen;
};

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

  win = MEM_CALLOC_I(device->allocator, 1, sizeof(struct wm_window));
  if(!win) {
    wm_err = WM_MEMORY_ERROR;
    goto error;
  }
  win->fullscreen = desc->fullscreen;

  width = (int)desc->width;
  height = (int)desc->height;
  mode = desc->fullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW;
  if(glfwOpenWindow(width, height, 8, 8, 8, 8, 24, 8, mode) == GL_FALSE)
    return WM_INTERNAL_ERROR;

exit:
  if(out_win)
    *out_win = win;
  return wm_err;
error:
  if(win) {
    MEM_FREE_I(device->allocator, win);
    win = NULL;
  }
  goto exit;
}

EXPORT_SYM enum wm_error
wm_free_window(struct wm_device* device, struct wm_window* win)
{
  if(!device || !win)
    return WM_INVALID_ARGUMENT;

  glfwCloseWindow();
  MEM_FREE_I(device->allocator, win);
  return WM_NO_ERROR;
}

EXPORT_SYM enum wm_error
wm_swap(struct wm_device* device UNUSED, struct wm_window* win UNUSED)
{
  if(!device || !win)
    return WM_INVALID_ARGUMENT;
  glfwSwapBuffers();
  return WM_NO_ERROR;
}

EXPORT_SYM enum wm_error
wm_get_window_desc
  (struct wm_device* dev, 
   struct wm_window* win,
   struct wm_window_desc* desc)
{
  int width = 0;
  int height = 0;

  if(!dev || !win || !desc)
    return WM_INVALID_ARGUMENT;

  glfwGetWindowSize(&width, &height);
  assert(width >= 0 && height >= 0);

  desc->width = (unsigned int)width;
  desc->height = (unsigned int)height;
  desc->fullscreen = win->fullscreen;
  return WM_NO_ERROR;
}
