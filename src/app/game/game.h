#ifndef GAME_H
#define GAME_H

#include "app/game/game_error.h"
#include <stdbool.h>

struct app;
struct game;
struct mem_allocator;

extern enum game_error
game_create
  (struct app* app,
   struct mem_allocator* allocator, /* May be NULL. */
   struct game** out_game);

extern enum game_error
game_free
  (struct game* game);

extern enum game_error
game_run
  (struct game* game,
   bool* keep_running);

#endif /* GAME_H. */

