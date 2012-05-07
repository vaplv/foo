#include "app/core/app.h"
#include "app/core/app_view.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include "window_manager/wm_device.h"
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char** argv)
{
  struct app_args args = { NULL, NULL, NULL, NULL };
  struct app* app = NULL;
  struct app_view* view = NULL;
  struct wm_device* wm = NULL;
  bool b = false;

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    return -1;
  }
  args.render_driver = argv[1];

  CHECK(app_init(NULL, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_init(&args, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_init(NULL, &app), APP_INVALID_ARGUMENT);
  CHECK(app_init(&args, &app), APP_NO_ERROR);

  CHECK(app_run(NULL, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_run(app, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_run(NULL, &b), APP_INVALID_ARGUMENT);
  CHECK(app_run(app, &b), APP_NO_ERROR);

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

  CHECK(app_cleanup(NULL), APP_INVALID_ARGUMENT);
  CHECK(app_cleanup(app), APP_NO_ERROR);

  CHECK(app_log(NULL, APP_NB_LOG_TYPES, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_log(app, APP_NB_LOG_TYPES, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_log(NULL, APP_LOG_ERROR, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_log(app, APP_LOG_ERROR, NULL), APP_INVALID_ARGUMENT);
  CHECK(app_log(NULL, APP_NB_LOG_TYPES, "msg\n"), APP_INVALID_ARGUMENT);
  CHECK(app_log(app, APP_NB_LOG_TYPES, "msg\n"), APP_INVALID_ARGUMENT);
  CHECK(app_log(NULL, APP_LOG_ERROR, "msg\n"), APP_INVALID_ARGUMENT);
  CHECK(app_log(app, APP_LOG_ERROR, "msg\n"), APP_NO_ERROR);
  CHECK(app_log(app, APP_LOG_WARNING, "msg %d\n", 2), APP_NO_ERROR);

  CHECK(app_ref_get(NULL), APP_INVALID_ARGUMENT);
  CHECK(app_ref_get(app), APP_NO_ERROR);
  CHECK(app_ref_put(NULL), APP_INVALID_ARGUMENT);
  CHECK(app_ref_put(app), APP_NO_ERROR);
  CHECK(app_ref_put(app), APP_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}
