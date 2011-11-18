#include "app/core/app_error.h"
#include "app/core/app.h"
#include "app/game/game.h"
#include "sys/mem_allocator.h"
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

static void
app_exit(void)
{
  struct mem_sys_info mem_info;
  mem_sys_info(&mem_info);
  printf
    ("Heap summary:\n"
     "  total heap size: %zu bytes\n"
     "  in use size at exit: %zu bytes\n",
     mem_info.total_size,
     mem_info.used_size);

  if(MEM_ALLOCATED_SIZE() != 0)
  {
    char dump[BUFSIZ];
    MEM_DUMP(dump, BUFSIZ, NULL);
    fprintf(stderr, "Leaks summary:\n%s\n", dump);
  }
}

int
main(int argc, char** argv)
{
  char buffer[BUFSIZ];
  struct app_args args;
  struct mem_allocator engine_allocator;
  struct mem_allocator game_allocator;
  struct app* app = NULL;
  struct game* game = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum game_error game_err = GAME_NO_ERROR;
  int err = 0;
  bool keep_running = true;

  memset(&args, 0, sizeof(struct app_args));

  /* Parse the argument list. */
  err = parse_args(argc, argv, &args);
  if(err == 1) {
    err = 0;
    goto exit;
  } else if(err == -1) {
    goto error;
  }

  atexit(app_exit);

  /* Initialize the application modules. */
  mem_init_proxy_allocator("engine", &engine_allocator, &mem_default_allocator);
  args.allocator = &engine_allocator;
  app_err = app_init(&args, &app);
  if(app_err != APP_NO_ERROR) {
    err = -1;
    goto error;
  }
  mem_init_proxy_allocator("game", &game_allocator, &mem_default_allocator);
  game_err = game_create(&game_allocator, &game);
  if(game_err != GAME_NO_ERROR) {
    err = -1;
    goto error;
  }

  /* Run the application. */
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
    if(MEM_ALLOCATED_SIZE_I(&engine_allocator)) {
      MEM_DUMP_I(&engine_allocator, buffer, sizeof(buffer), NULL);
      printf("Engine leaks summary:\n%s\n", buffer);
    }
    mem_shutdown_proxy_allocator(&engine_allocator);
  }
  if(game) {
    app_err = game_free(game);
    assert(app_err = APP_NO_ERROR);
    if(MEM_ALLOCATED_SIZE_I(&game_allocator)) {
      MEM_DUMP_I(&game_allocator, buffer, sizeof(buffer), NULL);
      printf("Game leaks summary:\n%s\n", buffer);
    }
    mem_shutdown_proxy_allocator(&game_allocator);
  }
  return err;

error:
  goto exit;
}

