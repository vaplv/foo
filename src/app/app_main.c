#include "app/editor/edit_context.h"
#include "app/core/app_core.h"
#include "app/game/game.h"
#include "sys/mem_allocator.h"
#include "window_manager/wm_input.h"
#include <assert.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum mode {
  MODE_NONE = 0,
  EDIT_MODE,
  GAME_MODE,
  TERM_MODE,
};

struct main {
  struct app* app;
  struct edit_context* edit;
  struct game* game;
  enum mode mode;
};

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
main_key_clbk(enum wm_key key, enum wm_state state, void* data)
{
  enum mode* mode = data;
  assert(mode);

  if(state != WM_PRESS)
    return;

  switch(key) {
    case WM_KEY_F1:
      *mode = (-(*mode != EDIT_MODE)) & EDIT_MODE;
      break;
    case WM_KEY_F2:
      *mode = (-(*mode != GAME_MODE)) & GAME_MODE;
      break;
    case WM_KEY_SQUARE:
      *mode = (-(*mode != TERM_MODE)) & TERM_MODE;
      break;
    default:
      /* Do nothing */
      break;
  }
}

static void
process_inputs(struct main* app_main)
{
  assert(app_main);
  EDIT(enable_inputs(app_main->edit, app_main->mode == EDIT_MODE)); 
  GAME(enable_inputs(app_main->game, app_main->mode == GAME_MODE));
  APP(enable_term(app_main->app, app_main->mode == TERM_MODE));
}

static void
app_exit(void)
{
  if(MEM_ALLOCATED_SIZE(&mem_default_allocator) != 0) {
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
  struct main app_main;
  struct mem_allocator edit_allocator;
  struct mem_allocator engine_allocator;
  struct mem_allocator game_allocator;
  struct wm_device* wm = NULL;
  const char* term_font_path = NULL;
  const char* model_path = NULL;
  const char* render_driver_path = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum edit_error edit_err = EDIT_NO_ERROR;
  enum game_error game_err = GAME_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;
  int err = 0;
  bool keep_running = true;

  memset(&args, 0, sizeof(struct app_args));
  memset(&app_main, 0, sizeof(struct main));

  mem_init_proxy_allocator("edit", &edit_allocator, &mem_default_allocator);
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
  app_err = app_init(&args, &app_main.app);
  if(app_err != APP_NO_ERROR) {
    fprintf(stderr, "Error in initializing the engine.\n");
    err = -1;
    goto error;
  }
  edit_err = edit_create_context(app_main.app, &edit_allocator, &app_main.edit);
  if(edit_err != EDIT_NO_ERROR) {
    fprintf(stderr, "Error in creating the edit context.\n");
    err = -1;
    goto error;
  }
  game_err = game_create(app_main.app, &game_allocator, &app_main.game);
  if(game_err != GAME_NO_ERROR) {
    fprintf(stderr, "Error in creating the game.\n");
    err = -1;
    goto error;
  }
  APP(get_window_manager_device(app_main.app, &wm));
  wm_err = wm_attach_key_callback(wm, main_key_clbk, &app_main.mode);
  if(wm_err != WM_NO_ERROR) {
    fprintf(stderr, "Error attaching the main key callback.\n");
    err = -1;
    goto error;
  }

  /* Run the application. */
  while(keep_running) {
    bool keep_game_running = false;
    bool keep_app_running = false;

    process_inputs(&app_main);

    /* Flush the app input events. */
    app_err = app_flush_events(app_main.app);
    if(app_err != APP_NO_ERROR) {
      fprintf(stderr, "Error processing app input events.\n");
      goto error;
    }
    game_err = game_run(app_main.game, &keep_game_running);
    if(game_err != GAME_NO_ERROR) {
      fprintf(stderr, "Error running the game.\n");
      goto error;
    }
    edit_err = edit_run(app_main.edit);
    if(edit_err != EDIT_NO_ERROR) {
      fprintf(stderr, "Error running the editor.\n");
      goto error;
    }
    app_err = app_run(app_main.app, &keep_app_running);
    if(app_err != APP_NO_ERROR) {
      fprintf(stderr, "Error running the engine.\n");
      goto error;
    }
    keep_running = keep_game_running && keep_app_running;
  }

exit:
  if(app_main.game) {
    GAME(free(app_main.game));
    if(MEM_ALLOCATED_SIZE(&game_allocator)) {
      MEM_DUMP(&game_allocator, buffer, sizeof(buffer));
      printf("Game leaks summary:\n%s\n", buffer);
    }
  }
  if(app_main.edit) {
   EDIT(context_ref_put(app_main.edit));
   if(MEM_ALLOCATED_SIZE(&edit_allocator)) {
     MEM_DUMP(&edit_allocator, buffer, sizeof(buffer));
     printf("Editor leaks summary:\n%s\n", buffer);
   }
  }
  if(app_main.app) {
    APP(cleanup(app_main.app));
    APP(ref_put(app_main.app));
    if(MEM_ALLOCATED_SIZE(&engine_allocator)) {
      MEM_DUMP(&engine_allocator, buffer, sizeof(buffer));
      printf("Engine leaks summary:\n%s\n", buffer);
    }
  }
  mem_shutdown_proxy_allocator(&edit_allocator);
  mem_shutdown_proxy_allocator(&engine_allocator);
  mem_shutdown_proxy_allocator(&game_allocator);
  return err;

error:
  fprintf(stderr, "Unexpected error\n");
  goto exit;
}

