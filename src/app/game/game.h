#ifndef GAME_H
#define GAME_H

#include "app/game/game_error.h"
#include <stdbool.h>

struct app;
struct game;

extern enum game_error
game_create
  (struct game** out_game);

extern enum game_error
game_free
  (struct game* game);

extern enum game_error
game_run
  (struct game* game,
   struct app* app,
   bool* keep_running);

#endif /* GAME_H. */

