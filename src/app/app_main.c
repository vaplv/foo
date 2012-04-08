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
parse_args
  (int argc, 
   char** argv, 
   const char** model_path,
   const char** render_driver_path,
   const char** term_font_path)
{
  int err = 0;
  int i = 0;

  assert(argc >= 1 && model_path && render_driver_path);

  if(argc == 1)
    goto usage;

  while(-1 != (i = getopt(argc, argv, "f:r:m:h"))) {
    switch(i) {
      case 'f':
        *term_font_path = optarg;
        break;
      case 'h':
        goto usage;
        break;
      case 'm':
        *model_path = optarg;
        break;
      case 'r':
        *render_driver_path = optarg;
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
  if(NULL == *render_driver_path)
    goto usage;

exit:
  return err;
usage:
  printf
    ("Usage: %s -r RENDER_DRIVER [-f TERM_FONT] [-m MODEL]\n"
     "  -f  Define the TERM_FONT to use.\n" 
     "  -m  Load MODEL at the launch of the application.\n"
     "  -r  Define the RENDER_DRIVER to use.\n",
     argv[0]);
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

  if(MEM_ALLOCATED_SIZE(&mem_default_allocator) != 0)
  {
    char dump[BUFSIZ];
    MEM_DUMP(&mem_default_allocator, dump, BUFSIZ);
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
  const char* term_font_path = NULL;
  const char* model_path = NULL;
  const char* render_driver_path = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum game_error game_err = GAME_NO_ERROR;
  int err = 0;
  bool keep_running = true;

  memset(&args, 0, sizeof(struct app_args));
  
  mem_init_proxy_allocator("engine", &engine_allocator, &mem_default_allocator);
  mem_init_proxy_allocator("game", &game_allocator, &mem_default_allocator);
 
  /* Parse the argument list. */
  err = parse_args
    (argc, argv, &model_path, &render_driver_path, &term_font_path);
  if(err == 1) {
    err = 0;
    goto exit;
  } else if(err == -1) {
    goto error;
  }
  atexit(app_exit);

  /* Initialize the application modules. */
  args.allocator = &engine_allocator;
  args.model = model_path;
  args.render_driver = render_driver_path;
  args.term_font = term_font_path;
  app_err = app_init(&args, &app);
  if(app_err != APP_NO_ERROR) {
    fprintf(stderr, "Error in initializing the engine.\n");
    err = -1;
    goto error;
  }
  game_err = game_create(app, &game_allocator, &game);
  if(game_err != GAME_NO_ERROR) {
    fprintf(stderr, "Error in creating the game.\n");
    err = -1;
    goto error;
  }
  /* Run the application. */
  while(keep_running) {
    if(keep_running) {
      bool keep_game_running = false;
      bool keep_app_running = false;
      game_err = game_run(game, &keep_game_running);
      if(game_err != GAME_NO_ERROR)
        goto error;

      app_err = app_run(app, &keep_app_running);
      if(app_err != APP_NO_ERROR) {
        fprintf(stderr, "Error running the application.\n");
        goto error; 
      }
      keep_running = keep_game_running && keep_app_running;
    }
  }

exit:
 if(game) {
    GAME(free(game));
    if(MEM_ALLOCATED_SIZE(&game_allocator)) {
      MEM_DUMP(&game_allocator, buffer, sizeof(buffer));
      printf("Game leaks summary:\n%s\n", buffer);
    }
  }
  if(app) {
    APP(cleanup(app));
    APP(ref_put(app));
    if(MEM_ALLOCATED_SIZE(&engine_allocator)) {
      MEM_DUMP(&engine_allocator, buffer, sizeof(buffer));
      printf("Engine leaks summary:\n%s\n", buffer);
    }
  }
  mem_shutdown_proxy_allocator(&engine_allocator);
  mem_shutdown_proxy_allocator(&game_allocator);
  return err;

error:
  goto exit;
}

