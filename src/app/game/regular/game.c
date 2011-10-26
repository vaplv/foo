#include "app/core/app.h"
#include "app/core/app_view.h"
#include "app/game/regular/game_c.h"
#include "app/game/game.h"
#include "sys/sys.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_input.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct user_command {
  signed char move_right;
  signed char move_up;
  signed char move_forward;
  struct {
    unsigned int escape : 1;
  } flag;
};

/*******************************************************************************
 *
 * Helper functions
 *
 ******************************************************************************/
static enum game_error
process_user_command
  (struct game* game,
   struct app* app,
   struct user_command* usr_cmd)
{
  struct wm_device* wm = NULL;
  enum wm_state state = WM_STATE_UNKNOWN;

  assert(game && app && usr_cmd);

  APP(get_window_manager_device(app, &wm));
  WM(flush_events(wm));

  WM(get_key_state(wm, WM_KEY_Z, &state));
  if(state == WM_PRESS) {
    usr_cmd->move_forward += usr_cmd->move_forward < SCHAR_MAX;
  }
  WM(get_key_state(wm, WM_KEY_S, &state));
  if(state == WM_PRESS) {
    usr_cmd->move_forward -= usr_cmd->move_forward > SCHAR_MIN;
  }
  WM(get_key_state(wm, WM_KEY_Q, &state));
  if(state == WM_PRESS) {
    usr_cmd->move_right += usr_cmd->move_right < SCHAR_MAX;
  }
  WM(get_key_state(wm, WM_KEY_D, &state));
  if(state == WM_PRESS) {
    usr_cmd->move_right -= usr_cmd->move_right > SCHAR_MIN;
  }
  WM(get_key_state(wm, WM_KEY_SPACE, &state));
  if(state == WM_PRESS) {
    usr_cmd->move_up -= usr_cmd->move_up > SCHAR_MIN;
  }
  WM(get_key_state(wm, WM_KEY_C, &state));
  if(state == WM_PRESS) {
    usr_cmd->move_up += usr_cmd->move_up < SCHAR_MAX;
  }
  WM(get_key_state(wm, WM_KEY_ESC, &state));
  if(state == WM_PRESS) {
    usr_cmd->flag.escape = 1;
  }

  return GAME_NO_ERROR;
}

static enum game_error
update_view
  (struct game* game,
   struct app* app,
   const struct user_command* usr_cmd)
{
  struct app_view* view = NULL;
  const float move_scale = 0.01f;

  assert(game && app && usr_cmd);

  APP(get_main_view(app, &view));
  APP(view_translate
    (app,
     view,
     move_scale * (float)usr_cmd->move_right,
     move_scale * (float)usr_cmd->move_up,
     move_scale * (float)usr_cmd->move_forward));

  return GAME_NO_ERROR;
}

/*******************************************************************************
 *
 * Game functions.
 *
 ******************************************************************************/
EXPORT_SYM enum game_error
game_create(struct game** out_game)
{
  struct game* game = NULL;
  enum game_error game_err = GAME_NO_ERROR;

  if(!out_game) {
    game_err = GAME_INVALID_ARGUMENT;
    goto error;
  }
  game = calloc(1, sizeof(struct game));
  if(!game) {
    game_err = GAME_MEMORY_ERROR;
    goto error;
  }

exit:
  if(out_game)
    *out_game = game;
  return game_err;

error:
  if(game) {
    free(game);
    game = NULL;
  }
  goto exit;
}

EXPORT_SYM enum game_error
game_free(struct game* game)
{
  if(!game)
    return GAME_INVALID_ARGUMENT;
  free(game);
  return GAME_NO_ERROR;
}

EXPORT_SYM enum game_error
game_run(struct game* game, struct app* app, bool* keep_running)
{
  struct user_command usr_cmd;
  enum game_error game_err = GAME_NO_ERROR;

  memset(&usr_cmd, 0, sizeof(struct user_command));

  if(!game || !app || !keep_running) {
    game_err = GAME_INVALID_ARGUMENT;
    goto error;
  }

  game_err = process_user_command(game, app, &usr_cmd);
  if(game_err != GAME_NO_ERROR)
    goto error;

  game_err = update_view(game, app, &usr_cmd);
  if(game_err != GAME_NO_ERROR)
    goto error;

  *keep_running = !usr_cmd.flag.escape;

exit:
  return game_err;

error:
  goto exit;
}

