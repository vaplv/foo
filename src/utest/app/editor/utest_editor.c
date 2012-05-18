#include "app/core/app.h"
#include "app/editor/edit_context.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"

int
main(int argc, char** argv)
{
  struct app_args args = { NULL, NULL, NULL, NULL };
  struct app* app = NULL;
  struct edit_context* edit = NULL;

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    return -1;
  }
  args.render_driver = argv[1];

  CHECK(app_init(&args, &app), APP_NO_ERROR);
  
  CHECK(edit_create_context(NULL, NULL, NULL), EDIT_INVALID_ARGUMENT);
  CHECK(edit_create_context(app, NULL, NULL), EDIT_INVALID_ARGUMENT);
  CHECK(edit_create_context(NULL, NULL, &edit), EDIT_INVALID_ARGUMENT);
  CHECK(edit_create_context(app, NULL, &edit), EDIT_NO_ERROR);

  CHECK(edit_run(NULL), EDIT_INVALID_ARGUMENT);
  CHECK(edit_run(edit), EDIT_NO_ERROR);

  CHECK(edit_context_ref_get(NULL), EDIT_INVALID_ARGUMENT);
  CHECK(edit_context_ref_get(edit), EDIT_NO_ERROR);
  CHECK(edit_context_ref_put(NULL), EDIT_INVALID_ARGUMENT);
  CHECK(edit_context_ref_put(edit), EDIT_NO_ERROR);
  CHECK(edit_context_ref_put(edit), EDIT_NO_ERROR);

  CHECK(app_ref_put(app), APP_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}
