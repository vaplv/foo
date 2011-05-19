#include "window_manager/wm.h"
#include "sys/sys.h"
#include <GL/glfw.h>
#include <stdlib.h>

/* Device. */
struct wm_device {
  char dummy;
};

EXPORT_SYM int
wm_create_device(struct wm_device** out_device)
{
  int err = 0;
  struct wm_device* device = NULL;
  if(!out_device)
    goto error;

  device = malloc(sizeof(struct wm_device));
  if(!device)
    goto error;

  if(glfwInit() == GL_FALSE)
    goto error;

  *out_device = device;

exit:
  return err;

error:
  free(device);
  err = -1;
  goto exit;
}

EXPORT_SYM int
wm_free_device(struct wm_device* device)
{
  if(!device)
    return -1;

  free(device);
  glfwTerminate();
  return 0;
}

/* Window. */
struct wm_window {
  char dummy;
};

EXPORT_SYM int
wm_create_window
  (struct wm_device* device,
   struct wm_window** out_win,
   struct wm_window_desc* desc)
{
  struct wm_window* win = NULL;

  if(!device || !out_win || !desc)
    return -1;

  int width = desc->width;
  int height = desc->height;
  int mode = desc->fullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW;
  if(glfwOpenWindow(width, height, 8, 8, 8, 8, 24, 8, mode) == GL_FALSE) {
    return -1;
  }

  /* Do not physically create the window. */
  *out_win = win;

  return 0;
}

EXPORT_SYM int
wm_free_window(struct wm_device* device, struct wm_window* win)
{
  /* The window must be null since it is never allocatedd (Refer to the
   * wm_create_windowfunction). */
  if(!device || win != NULL)
    return -1;

  glfwCloseWindow();
  return 0;
}

EXPORT_SYM int
wm_swap(struct wm_device* device, struct wm_window* win)
{
  glfwSwapBuffers();
  return 0;
}

