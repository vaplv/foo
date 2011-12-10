#include "app/core/app.h"
#include "app/core/app_view.h"
#include "app/game/regular/game_c.h"
#include "app/game/game.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_input.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct user_command {
  signed char move_right;
  signed char move_up;
  signed char move_forward;
  signed char pitch;
  signed char yaw;
  signed char roll;
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
   struct user_command* usr_cmd)
{
  struct wm_device* wm = NULL;
  enum wm_state state = WM_STATE_UNKNOWN;

  assert(game && usr_cmd);

  APP(get_window_manager_device(game->app, &wm));
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
  WM(get_key_state(wm, WM_KEY_J, &state));
  if(state == WM_PRESS) {
    usr_cmd->pitch += usr_cmd->pitch < SCHAR_MAX;
  }
  WM(get_key_state(wm, WM_KEY_K, &state));
  if(state == WM_PRESS) {
    usr_cmd->pitch -= usr_cmd->pitch > SCHAR_MIN;
  }
  WM(get_key_state(wm, WM_KEY_H, &state));
  if(state == WM_PRESS) {
    usr_cmd->yaw += usr_cmd->pitch < SCHAR_MAX;
  }
  WM(get_key_state(wm, WM_KEY_L, &state));
  if(state == WM_PRESS) {
    usr_cmd->yaw -= usr_cmd->pitch > SCHAR_MIN;
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
   const struct user_command* usr_cmd)
{
  struct app_view* view = NULL;
  const float move_scale = 0.02f;
  const float rotate_scale = 0.00025f;

  assert(game && usr_cmd);

  APP(get_main_view(game->app, &view));
  APP(view_translate
    (view,
     move_scale * (float)usr_cmd->move_right,
     move_scale * (float)usr_cmd->move_up,
     move_scale * (float)usr_cmd->move_forward));
  APP(view_rotate
    (view,
     rotate_scale * (float)usr_cmd->pitch,
     rotate_scale * (float)usr_cmd->yaw,
     rotate_scale * (float)usr_cmd->roll));
  return GAME_NO_ERROR;
}

static bool
user_command_eq(struct user_command* cmd0, struct user_command* cmd1)
{
  return
        cmd0->move_right == cmd1->move_right
    &&  cmd0->move_up == cmd1->move_up
    &&  cmd0->move_forward == cmd1->move_forward
    &&  cmd0->pitch == cmd1->pitch
    &&  cmd0->yaw == cmd1->yaw
    &&  cmd0->roll == cmd1->roll
    &&  cmd0->flag.escape == cmd1->flag.escape;
}

/*******************************************************************************
 *
 * Game functions.
 *
 ******************************************************************************/
EXPORT_SYM enum game_error
game_create
  (struct app* app,
   struct mem_allocator* specific_allocator, 
   struct game** out_game)
{
  struct mem_allocator* allocator = NULL;
  struct game* game = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum game_error game_err = GAME_NO_ERROR;

  if(!app || !out_game) {
    game_err = GAME_INVALID_ARGUMENT;
    goto error;
  }

  allocator = specific_allocator ? specific_allocator : &mem_default_allocator;
  game = MEM_CALLOC(allocator, 1, sizeof(struct game));
  if(!game) {
    game_err = GAME_MEMORY_ERROR;
    goto error;
  }
  game->allocator = allocator;

  app_err = app_ref_get(app);
  if(!app_err) {
    game_err = app_to_game_error(app_err);
    goto error;
  }
  game->app = app;

exit:
  if(out_game)
    *out_game = game;
  return game_err;

error:
  if(game) {
    if(game->app)
      APP(ref_put(game->app));
    MEM_FREE(allocator, game);
    game = NULL;
  }
  goto exit;
}

EXPORT_SYM enum game_error
game_free(struct game* game)
{
  struct mem_allocator* allocator = NULL;

  if(!game)
    return GAME_INVALID_ARGUMENT;

  APP(ref_put(game->app));
  allocator = game->allocator;
  MEM_FREE(allocator, game);

  return GAME_NO_ERROR;
}

EXPORT_SYM enum game_error
game_run(struct game* game, bool* keep_running)
{
  struct user_command usr_cmd0, usr_cmd1;
  enum game_error game_err = GAME_NO_ERROR;

  memset(&usr_cmd0, 0, sizeof(struct user_command));
  memset(&usr_cmd1, 0, sizeof(struct user_command));

  if(!game || !keep_running) {
    game_err = GAME_INVALID_ARGUMENT;
    goto error;
  }

  game_err = process_user_command(game, &usr_cmd1);
  if(game_err != GAME_NO_ERROR)
    goto error;

  if(user_command_eq(&usr_cmd0, &usr_cmd1) == false) {
    game_err = update_view(game, &usr_cmd1);
    if(game_err != GAME_NO_ERROR)
      goto error;
  }

  *keep_running = !usr_cmd1.flag.escape;

exit:
  return game_err;

error:
  goto exit;
}

/*******************************************************************************
 *
 * Private functions.
 *
 ******************************************************************************/
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

