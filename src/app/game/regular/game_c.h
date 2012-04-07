#ifndef GAME_C_H
#define GAME_C_H

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

struct app;
struct mem_allocator;

struct game {
  struct inputs {
    signed char move_right;
    signed char move_up;
    signed char move_forward;
    signed char pitch;
    signed char yaw;
    signed char roll;
    struct post_event {
      unsigned int exit : 1;
      unsigned int terminal : 1;
    } post;
  } inputs;
  struct app* app;
  struct mem_allocator* allocator;
};

#endif /* GAME_C_H */

