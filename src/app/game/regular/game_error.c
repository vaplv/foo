#include "app/game/regular/game_error_c.h"

enum game_error
app_to_game_error(enum app_error app_err)
{
  enum game_error game_err = GAME_NO_ERROR;
  switch(app_err) {
    case APP_INVALID_ARGUMENT:
      game_err = GAME_INVALID_ARGUMENT;
      break;
    case APP_MEMORY_ERROR:
      game_err = GAME_MEMORY_ERROR;
      break;
    case APP_NO_ERROR:
      game_err = GAME_NO_ERROR;
      break;
    default:
      game_err = GAME_UNKNOWN_ERROR;
      break;
  }
  return game_err;
}

enum game_error
wm_to_game_error(enum wm_error wm_err)
{
  enum game_error game_err = GAME_NO_ERROR;
  switch(wm_err) {
    case WM_INVALID_ARGUMENT:
      game_err = GAME_INVALID_ARGUMENT;
      break;
    case WM_MEMORY_ERROR:
      game_err = GAME_MEMORY_ERROR;
      break;
    case WM_NO_ERROR:
      game_err = GAME_NO_ERROR;
      break;
    default:
      game_err = GAME_UNKNOWN_ERROR;
      break;
  }
  return game_err;
}

