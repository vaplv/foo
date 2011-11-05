#include "app/core/app.h"
#include "app/core/app_view.h"
#include "utest/utest.h"
#include "sys/sys.h"
#include "window_manager/wm_device.h"
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char** argv)
{
  struct app_args args = { NULL, NULL };
  struct app* app = NULL;
  struct app_view* view = NULL;
  struct wm_device* wm = NULL;
  struct sys* sys = NULL;

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    return -1;
  }
  args.render_driver = argv[1];

  CHECK(sys_init(&sys), SYS_NO_ERROR);

  CHECK(app_init(NULL, NULL, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_init(&args, NULL, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_init(NULL, NULL, &app), APP_INVALID_ARGUMENT);
  CHECK(app_init(&args, NULL, &app), APP_INVALID_ARGUMENT);
  CHECK(app_init(NULL, sys, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_init(&args, sys,NULL), APP_INVALID_ARGUMENT);
  CHECK(app_init(NULL, sys, &app), APP_INVALID_ARGUMENT);
  CHECK(app_init(&args, sys, &app), APP_NO_ERROR);

  CHECK(app_run(NULL), APP_INVALID_ARGUMENT);
  CHECK(app_run(app), APP_NO_ERROR);

  CHECK(app_get_window_manager_device(NULL, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_get_window_manager_device(app, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_get_window_manager_device(NULL, &wm), APP_INVALID_ARGUMENT);
  CHECK(app_get_window_manager_device(app, &wm), APP_NO_ERROR);
  NCHECK(wm, NULL);

  CHECK(app_get_main_view(NULL, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_get_main_view(app, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_get_main_view(NULL, &view), APP_INVALID_ARGUMENT);
  CHECK(app_get_main_view(app, &view), APP_NO_ERROR);
  NCHECK(view, NULL);

  CHECK(app_shutdown(NULL), APP_INVALID_ARGUMENT);
  CHECK(app_shutdown(app), APP_NO_ERROR);

  CHECK(sys_shutdown(sys), SYS_NO_ERROR);

  return 0;
}
