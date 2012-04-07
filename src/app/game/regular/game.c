#include "app/core/app.h"
#include "app/core/app_view.h"
#include "app/game/regular/game_c.h"
#include "app/game/regular/game_error_c.h"
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

/*******************************************************************************
 *
 * Helper functions
 *
 ******************************************************************************/
static void
game_key_callback(enum wm_key key, enum wm_state state, void* data)
{
  struct inputs* inputs = data;
  assert(data);

  if(state != WM_PRESS)
    return;

  switch(key) {
    case WM_KEY_ESC:
      inputs->post.exit = 1;
      break;
    case WM_KEY_SQUARE:
      inputs->post.terminal = 1;
      break;
    default:
      break;
  }
}
static void
update_game_inputs(struct game* game)
{
  struct wm_device* wm = NULL;
  enum wm_state state = WM_STATE_UNKNOWN;

  APP(get_window_manager_device(game->app, &wm));
  
  WM(get_key_state(wm, WM_KEY_Z, &state));
  game->inputs.move_forward += 
    (state == WM_PRESS) & (game->inputs.move_forward < SCHAR_MAX);
  WM(get_key_state(wm, WM_KEY_S, &state));
  game->inputs.move_forward -= 
    (state == WM_PRESS) & (game->inputs.move_forward > SCHAR_MIN);
  WM(get_key_state(wm, WM_KEY_Q, &state));
  game->inputs.move_right += 
    (state == WM_PRESS) & (game->inputs.move_right < SCHAR_MAX);
  WM(get_key_state(wm, WM_KEY_D, &state));
  game->inputs.move_right -= 
    (state == WM_PRESS) & (game->inputs.move_right > SCHAR_MIN);
  WM(get_key_state(wm, WM_KEY_SPACE, &state));
  game->inputs.move_up -= 
     (state == WM_PRESS) & (game->inputs.move_up > SCHAR_MIN);
  WM(get_key_state(wm, WM_KEY_C, &state));
  game->inputs.move_up +=
    (state == WM_PRESS) & (game->inputs.move_up < SCHAR_MAX);
  WM(get_key_state(wm, WM_KEY_J, &state));
  game->inputs.pitch +=
    (state == WM_PRESS) & (game->inputs.pitch < SCHAR_MAX);
  WM(get_key_state(wm, WM_KEY_K, &state));
  game->inputs.pitch -=
    (state == WM_PRESS) & (game->inputs.pitch > SCHAR_MIN);
  WM(get_key_state(wm, WM_KEY_H, &state));
  game->inputs.yaw +=
    (state == WM_PRESS) & (game->inputs.pitch < SCHAR_MAX);
  WM(get_key_state(wm, WM_KEY_L, &state));
  game->inputs.yaw -=
    (state == WM_PRESS) & (game->inputs.pitch > SCHAR_MIN);
}

static enum game_error
process_inputs(struct game* game)
{
  struct wm_device* wm = NULL;
  enum game_error game_err = GAME_NO_ERROR;
  bool term_state = false;
  assert(game);

  /* Reset inputs. */
  memset(&game->inputs, 0, sizeof(struct inputs));

  /* Flush user inputs. */
  APP(get_window_manager_device(game->app, &wm));
  WM(flush_events(wm));

  /* Update terminal state. */
  APP(is_term_enabled(game->app, &term_state));
  if(0 != game->inputs.post.terminal) {
    enum app_error app_err = APP_NO_ERROR;
    app_err = app_enable_term(game->app, !term_state);
    if(APP_NO_ERROR != app_err) {
      game_err = app_to_game_error(app_err);
      goto error;
    }
  }

  if(false == term_state) {
    update_game_inputs(game);
  } else {
    game->inputs.post.exit = 0;
  }

  /* Update the view. */
  if(0 != game->inputs.move_right
  || 0 != game->inputs.move_up
  || 0 != game->inputs.move_forward
  || 0 != game->inputs.pitch
  || 0 != game->inputs.yaw 
  || 0 != game->inputs.roll) {
    struct app_view* view = NULL;
    const float move_scale = 0.02f;
    const float rotate_scale = 0.00025f;

    APP(get_main_view(game->app, &view));
    APP(view_translate
      (view,
       move_scale * (float)game->inputs.move_right,
       move_scale * (float)game->inputs.move_up,
       move_scale * (float)game->inputs.move_forward));
    APP(view_rotate
      (view,
       rotate_scale * (float)game->inputs.pitch,
       rotate_scale * (float)game->inputs.yaw,
       rotate_scale * (float)game->inputs.roll));
  }

exit:
  return game_err;
error:
  goto exit;
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
  struct wm_device* wm = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum game_error game_err = GAME_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;

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
  APP(get_window_manager_device(game->app, &wm));
  wm_err = wm_attach_key_callback(wm, game_key_callback, &game->inputs);
  if(wm_err != WM_NO_ERROR) {
    game_err = wm_to_game_error(wm_err);
    goto error;
  }

exit:
  if(out_game)
    *out_game = game;
  return game_err;

error:
  if(game) {
    GAME(free(game));
    game = NULL;
  }
  goto exit;
}

EXPORT_SYM enum game_error
game_free(struct game* game)
{
  if(!game)
    return GAME_INVALID_ARGUMENT;

  if(game->app) {
    struct wm_device* wm = NULL;
    bool b = false;
    APP(get_window_manager_device(game->app, &wm));
    WM(is_key_callback_attached(wm, game_key_callback, &game->inputs, &b));
    if(b)
      WM(detach_key_callback(wm, game_key_callback, &game->inputs));
    APP(ref_put(game->app));
  }
  MEM_FREE(game->allocator, game);
  return GAME_NO_ERROR;
}

EXPORT_SYM enum game_error
game_run(struct game* game, bool* keep_running)
{
  enum game_error game_err = GAME_NO_ERROR;

  if(!game || !keep_running) {
    game_err = GAME_INVALID_ARGUMENT;
    goto error;
  }
  game_err = process_inputs(game);
  if(GAME_NO_ERROR != game_err)
    goto error;

  *keep_running = !game->inputs.post.exit;

exit:
  return game_err;
error:
  goto exit;
}

