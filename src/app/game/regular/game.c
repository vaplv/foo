#include "app/core/app_core.h"
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
  struct post_event* post = data;
  assert(data);

  if(state != WM_PRESS)
    return;

  switch(key) {
    case WM_KEY_ESC:
      post->exit = 1;
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
  enum game_error game_err = GAME_NO_ERROR;
  assert(game);

  /* Reset inputs. */
  memset(&game->inputs, 0, sizeof(struct inputs));
  update_game_inputs(game);

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

  return game_err;
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
  if(game->process_inputs == false) {
    *keep_running = true;
  } else {
    game_err = process_inputs(game);
    if(GAME_NO_ERROR != game_err)
      goto error;

    *keep_running = !game->post.exit;

    /* Reset post event. */
    memset(&game->post, 0, sizeof(struct post_event));
  }

exit:
  return game_err;
error:
  goto exit;
}

EXPORT_SYM enum game_error
game_enable_inputs(struct game* game, bool is_enable)
{
  struct wm_device* wm = NULL;
  bool is_clbk_attached = false;
  enum game_error game_err = GAME_NO_ERROR;

  if(!game) {
    game_err = GAME_INVALID_ARGUMENT;
    goto error;
  }
 
  if(game->process_inputs != is_enable) {
    enum wm_error wm_err = WM_NO_ERROR;
    APP(get_window_manager_device(game->app, &wm));
    WM(is_key_callback_attached
       (wm, game_key_callback, &game->post, &is_clbk_attached));

    if(is_enable && !is_clbk_attached) {
      wm_err = wm_attach_key_callback(wm, game_key_callback, &game->post);
      if(wm_err != WM_NO_ERROR) {
        game_err = wm_to_game_error(wm_err);
        goto error;
      }
    }
    if(!is_enable && is_clbk_attached) {
      wm_err = wm_detach_key_callback(wm, game_key_callback, &game->post);
      if(wm_err != WM_NO_ERROR) {
        game_err = wm_to_game_error(wm_err);
        goto error;
      }
    }
    game->process_inputs = is_enable;
  }

exit:
  return game_err;
error:
  if(game) {
    if(wm) {
      bool b = false;
      WM(is_key_callback_attached(wm, game_key_callback, &game->post, &b));
      if(!b && is_clbk_attached) {
        WM(attach_key_callback(wm, game_key_callback, &game->post));
      }
      if(b && !is_clbk_attached) {
        WM(detach_key_callback(wm, game_key_callback, &game->post));
      }
    }
  }
  goto exit;
}

