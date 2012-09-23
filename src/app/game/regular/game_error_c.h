#ifndef GAME_ERROR_C_H
#define GAME_ERROR_C_H

#include "app/core/app_error.h"
#include "app/game/game_error.h"
#include "sys/sys.h"
#include "window_manager/wm_error.h"

LOCAL_SYM enum game_error
app_to_game_error
  (enum app_error app_err);

LOCAL_SYM enum game_error
wm_to_game_error
  (enum wm_error wm_err);

#endif /* GAME_ERROR_C_H */

