#include "app/core/app_error.h"
#include "app/core/app.h"
#include "app/game/game.h"
#include <assert.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* return 1 if the `-h' option was parsed, -1 if a parsing error occurs and 0
 * otherwise. */
static int
parse_args(int argc, char** argv, struct app_args* args)
{
  int err = 0;
  int i = 0;
  assert(argc >= 1);

  if(argc == 1)
    goto usage;

  while(-1 != (i = getopt(argc, argv, "r:m:h"))) {
    switch(i) {
      case 'h':
        goto usage;
        break;
      case 'm':
        args->model = optarg;
        break;
      case 'r':
        args->render_driver = optarg;
        break;
      case ':': /* missing argument. */
      case '?': /* unrecognized option. */
        goto error;
        break;
      default:
        assert(0);
        break;
    }
  }

exit:
  return err;
usage:
  printf("Usage: %s [-r RENDER_DRIVER] [-m MODEL]\n", argv[0]);
  err = 1;
  goto exit;
error:
  fprintf(stderr, "Try `%s -h' for more information\n", argv[0]);
  err = -1;
  goto exit;
}

int
main(int argc, char** argv)
{
  struct app_args args;
  struct app* app = NULL;
  struct game* game = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum game_error game_err = GAME_NO_ERROR;
  int err = 0;
  bool keep_running = true;

  memset(&args, 0, sizeof(struct app_args));

  err = parse_args(argc, argv, &args);
  if(err == 1) {
    err = 0;
    goto exit;
  } else if(err == -1) {
    goto error;
  }

  app_err = app_init(&args, &app);
  if(app_err != APP_NO_ERROR) {
    err = -1;
    goto error;
  }
  game_err = game_create(&game);
  if(game_err != GAME_NO_ERROR) {
    err = -1;
    goto error;
  }
  while(keep_running) {
    game_err = game_run(game, app, &keep_running);
    if(game_err != GAME_NO_ERROR)
      goto error;

    app_err = app_run(app);
    if(app_err != APP_NO_ERROR)
      goto error;
  }

exit:
  if(app) {
    app_err = app_shutdown(app);
    assert(app_err == APP_NO_ERROR);
  }
  if(game) {
    app_err = game_free(game);
    assert(app_err = APP_NO_ERROR);
  }
  return err;

error:
  goto exit;
}
