#include "app/core/app.h"
#include "app/game/game.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"

int
main(int argc, char** argv)
{
  struct app_args args = { NULL, NULL, NULL, NULL };
  struct app* app = NULL;
  struct game* game = NULL;
  bool b = false;

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    return -1;
  }
  args.render_driver = argv[1];

  CHECK(app_init(&args, &app), APP_NO_ERROR);
  
  CHECK(game_create(NULL, NULL, NULL), GAME_INVALID_ARGUMENT);
  CHECK(game_create(NULL, NULL, &game), GAME_INVALID_ARGUMENT);
  CHECK(game_create(app, NULL, NULL), GAME_INVALID_ARGUMENT);
  CHECK(game_create(app, NULL, &game), GAME_NO_ERROR);

  CHECK(game_run(NULL, NULL), GAME_INVALID_ARGUMENT);
  CHECK(game_run(game, NULL), GAME_INVALID_ARGUMENT);
  CHECK(game_run(NULL, NULL), GAME_INVALID_ARGUMENT);
  CHECK(game_run(game, NULL), GAME_INVALID_ARGUMENT);
  CHECK(game_run(NULL, &b), GAME_INVALID_ARGUMENT);
  CHECK(game_run(game, &b), GAME_NO_ERROR);

  CHECK(game_free(NULL), GAME_INVALID_ARGUMENT);
  CHECK(game_free(game), GAME_NO_ERROR);

  CHECK(app_ref_put(app), APP_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}
