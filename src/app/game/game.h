#ifndef GAME_H
#define GAME_H

#include "app/game/game_error.h"
#include "sys/sys.h"
#include <stdbool.h>

#ifndef NDEBUG
  #include <assert.h>
  #define GAME(func) assert(GAME_NO_ERROR == game_##func)
#else
  #define GAME(func) game_##func
#endif

#if defined(BUILD_GAME)
  #define GAME_API EXPORT_SYM
#else
  #define GAME_API IMPORT_SYM
#endif

struct app;
struct game;
struct mem_allocator;

GAME_API enum game_error
game_create
  (struct app* app,
   struct mem_allocator* allocator, /* May be NULL. */
   struct game** out_game);

GAME_API enum game_error
game_free
  (struct game* game);

GAME_API enum game_error
game_run
  (struct game* game,
   bool* keep_running);

GAME_API enum game_error
game_enable_inputs
  (struct game* game,
   bool is_enable);

#endif /* GAME_H. */

