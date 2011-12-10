#ifndef GAME_C_H
#define GAME_C_H

#include "app/core/app_error.h"
#include "app/game/game_error.h"
#include "sys/mem_allocator.h"

#ifndef NDEBUG
  #include <assert.h>
  #define APP(func) assert(APP_NO_ERROR == app_##func)
  #define WM(func) assert(WM_NO_ERROR == wm_##func)
  #define GAME(func) assert(GAME_NO_ERROR == game_##func)
#else
  #define APP(func) app_##func
  #define WM(func) wm_##func
  #define GAME(func) game_##func
#endif

struct game {
  struct app* app;
  struct mem_allocator* allocator;
};

enum game_error
app_to_game_error
  (enum app_error app_err);

#endif /* GAME_C_H */

