#include "app/core/app.h"
#include "app/game/game.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"

int
main(int argc, char** argv)
{
  struct app_args args = { NULL, NULL, NULL };
  struct app* app = NULL;
  struct game* game = NULL;
  bool b = false;

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    return -1;
  }
  args.render_driver = argv[1];

  CHECK(app_init(&args, &app), APP_NO_ERROR);

  CHECK(game_create(NULL, NULL), GAME_INVALID_ARGUMENT);
  CHECK(game_create(NULL, &game), GAME_NO_ERROR);

  CHECK(game_run(NULL, NULL, NULL), GAME_INVALID_ARGUMENT);
  CHECK(game_run(game, NULL, NULL), GAME_INVALID_ARGUMENT);
  CHECK(game_run(NULL, app, NULL), GAME_INVALID_ARGUMENT);
  CHECK(game_run(game, app, NULL), GAME_INVALID_ARGUMENT);
  CHECK(game_run(NULL, NULL, &b), GAME_INVALID_ARGUMENT);
  CHECK(game_run(game, NULL, &b), GAME_INVALID_ARGUMENT);
  CHECK(game_run(NULL, app, &b), GAME_INVALID_ARGUMENT);
  CHECK(game_run(game, app, &b), GAME_NO_ERROR);

  CHECK(game_free(NULL), GAME_INVALID_ARGUMENT);
  CHECK(game_free(game), GAME_NO_ERROR);

  CHECK(app_ref_put(app), APP_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}
