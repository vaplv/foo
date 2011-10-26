#ifndef GAME_C_H
#define GAME_C_H

#ifndef NDEBUG
  #include <assert.h>
  #define APP(func) assert(APP_NO_ERROR == app_##func)
  #define WM(func) assert(WM_NO_ERROR == wm_##func)
#else
  #define APP(func) app_##func
  #define WM(func) wm_##func
#endif

struct game {
  char dummy;
};

#endif /* GAME_C_H */

