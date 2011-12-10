#include "app/core/app.h"
#include "app/editor/edit.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"

int
main(int argc, char** argv)
{
  struct app_args args = { NULL, NULL, NULL };
  struct app* app = NULL;
  struct edit* edit = NULL;
  const char* gui_desc = NULL;
  bool b = false;

  if(argc != 3) {
    printf("usage: %s RB_DRIVER GUI_DESC\n", argv[0]);
    return -1;
  }
  args.render_driver = argv[1];
  gui_desc = argv[2];

  CHECK(app_init(&args, &app), APP_NO_ERROR);

  CHECK(edit_init(NULL, NULL, NULL, NULL), EDIT_INVALID_ARGUMENT);
  CHECK(edit_init(NULL, NULL, NULL, &edit), EDIT_INVALID_ARGUMENT);
  CHECK(edit_init(app, NULL, NULL, NULL), EDIT_INVALID_ARGUMENT);
  CHECK(edit_init(app, NULL, NULL, &edit), EDIT_INVALID_ARGUMENT);
  CHECK(edit_init(NULL, gui_desc, NULL, NULL), EDIT_INVALID_ARGUMENT);
  CHECK(edit_init(NULL, gui_desc, NULL, &edit), EDIT_INVALID_ARGUMENT);
  CHECK(edit_init(app, gui_desc, NULL, NULL), EDIT_INVALID_ARGUMENT);
  CHECK(edit_init(app, gui_desc, NULL, &edit), EDIT_NO_ERROR);

  CHECK(edit_run(NULL, NULL), EDIT_INVALID_ARGUMENT);
  CHECK(edit_run(NULL, &b), EDIT_INVALID_ARGUMENT);
  CHECK(edit_run(edit, NULL), EDIT_INVALID_ARGUMENT);
  CHECK(edit_run(edit, &b), EDIT_NO_ERROR);

  CHECK(edit_shutdown(NULL), EDIT_INVALID_ARGUMENT);
  CHECK(edit_shutdown(edit), EDIT_NO_ERROR);

  CHECK(app_ref_put(app), APP_NO_ERROR); 

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;
}
