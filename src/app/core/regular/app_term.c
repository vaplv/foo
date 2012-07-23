#include "app/core/regular/app_c.h"
#include "app/core/regular/app_command_buffer_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_term.h"
#include "app/core/app_command.h"
#include "app/core/app_command_buffer.h"
#include "renderer/rdr.h"
#include "renderer/rdr_term.h"
#include "window_manager/wm.h"
#include "window_manager/wm_input.h"
#include "window_manager/wm_window.h"
#include <limits.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
command_completion(struct term* term)
{
  struct app* app = NULL;
  const char* cmdbuf = NULL;
  const char** list = NULL;
  size_t cursor = 0;
  size_t len = 0;
  assert(term);

  app = CONTAINER_OF(term, struct app, term);

  APP(command_buffer_completion(app->term.cmdbuf, &len, &list));
  APP(get_command_buffer_string(app->term.cmdbuf, &cursor, &cmdbuf));
  RDR(clear_term(app->term.render_term, RDR_TERM_CMDOUT));
  RDR(term_print_string
    (app->term.render_term, RDR_TERM_CMDOUT, cmdbuf, RDR_TERM_COLOR_WHITE));
  RDR(term_translate_cursor(app->term.render_term, INT_MIN));
  RDR(term_translate_cursor(app->term.render_term, cursor));
  if(0 != len) {
    if(1 < len) {
      size_t i = 0;
      for(i = 0; i < len; ++i) {
        if(0 != i) {
          RDR(term_print_wchar
            (app->term.render_term,
             RDR_TERM_STDOUT,
             L'\t',
             RDR_TERM_COLOR_WHITE));
        }
        RDR(term_print_string
          (app->term.render_term,
           RDR_TERM_STDOUT,
           list[i],
           RDR_TERM_COLOR_WHITE));
      }
      RDR(term_print_wchar
        (app->term.render_term, RDR_TERM_STDOUT, L'\n', RDR_TERM_COLOR_WHITE));
    }
  }
}

static void
term_char_clbk(wchar_t wch, enum wm_state state, void* data)
{
  struct app* app = data;
  enum app_error app_err = APP_NO_ERROR;
  const char ch = (char)wch;
  assert(data);

  if(state != WM_PRESS || wch > 126 || wch < 32)
    goto exit;

  app_err = app_command_buffer_write_char(app->term.cmdbuf, ch);
  if(APP_NO_ERROR != app_err) {
    APP_PRINT_ERR
      (app->logger,
       "command buffer input error: `%s'\n",
       app_error_string(app_err));
    goto error;
  }
  RDR(term_print_wchar
    (app->term.render_term,
     RDR_TERM_CMDOUT,
     wch,
     RDR_TERM_COLOR_WHITE));
exit:
  return;
error:
  goto exit;
}

static void
term_key_clbk(enum wm_key key, enum wm_state state, void* data)
{
  const char* cstr = NULL;
  struct term* term = NULL;
  enum app_error app_err = APP_NO_ERROR;
  size_t cursor = 0;
  assert(data);

  if(state != WM_PRESS)
    return;

  term = &((struct app*)data)->term;
  switch(key) {
    case WM_KEY_ENTER:
      RDR(term_write_return(term->render_term));
      app_err = app_execute_command_buffer(term->cmdbuf);
      assert(APP_COMMAND_ERROR == app_err || APP_NO_ERROR == app_err);
      break;
    case WM_KEY_BACKSPACE:
      RDR(term_write_backspace(term->render_term));
      APP(command_buffer_write_backspace(term->cmdbuf));
      break;
    case WM_KEY_DEL:
      RDR(term_write_suppr(term->render_term));
      APP(command_buffer_write_suppr(term->cmdbuf));
      break;
    case WM_KEY_RIGHT:
      RDR(term_translate_cursor(term->render_term, 1));
      APP(command_buffer_move_cursor(term->cmdbuf, 1));
      break;
    case WM_KEY_LEFT:
      RDR(term_translate_cursor(term->render_term, -1));
      APP(command_buffer_move_cursor(term->cmdbuf, -1));
      break;
    case WM_KEY_END:
      RDR(term_translate_cursor(term->render_term, INT_MAX));
      APP(command_buffer_move_cursor(term->cmdbuf, INT_MAX));
      break;
    case WM_KEY_HOME:
      RDR(term_translate_cursor(term->render_term, INT_MIN));
      APP(command_buffer_move_cursor(term->cmdbuf, INT_MIN));
      break;
    case WM_KEY_UP:
    case WM_KEY_DOWN:
      if(WM_KEY_UP == key ) {
        APP(command_buffer_history_next(term->cmdbuf));
      } else {
        APP(command_buffer_history_prev(term->cmdbuf));
      }
      APP(get_command_buffer_string(term->cmdbuf, &cursor, &cstr));
      RDR(clear_term(term->render_term, RDR_TERM_CMDOUT));
      RDR(term_print_string
       (term->render_term, RDR_TERM_CMDOUT, cstr, RDR_TERM_COLOR_WHITE));
      RDR(term_translate_cursor(term->render_term, INT_MIN));
      RDR(term_translate_cursor(term->render_term, cursor));
      break;
    case WM_KEY_TAB:
      command_completion(term);
      break;
    default:
      break;
  }
}

/*******************************************************************************
 *
 * Command buffer functions.
 *
 ******************************************************************************/
enum app_error
app_init_term(struct app* app)
{
  struct wm_window_desc win_desc;
  enum app_error app_err = APP_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!app || !app->rdr.term_font || !app->wm.window) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  app_err = app_regular_create_command_buffer(app, &app->term.cmdbuf);
  if(APP_NO_ERROR != app_err)
    goto error;
  WM(get_window_desc(app->wm.window, &win_desc));
  rdr_err = rdr_create_term
    (app->rdr.system,
     app->rdr.term_font,
     win_desc.width,
     win_desc.height,
     &app->term.render_term);
  if(RDR_NO_ERROR != rdr_err) {
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }
  rdr_err = rdr_term_print_wstring
    (app->term.render_term, RDR_TERM_PROMPT, L"$ ", RDR_TERM_COLOR_GREEN);
  if(RDR_NO_ERROR != rdr_err) {
    app_err = rdr_to_app_error(rdr_err);
    goto error;
  }
  app->term.is_enabled = false;
exit:
  return app_err;
error:
  APP(shutdown_term(app));
  goto exit;
}

enum app_error
app_shutdown_term(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  if(!app) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  if(app->term.cmdbuf)
    APP(regular_command_buffer_ref_put(app->term.cmdbuf));
  if(app->term.render_term)
    RDR(term_ref_put(app->term.render_term));
  if(true == app->term.is_enabled)
    APP(enable_term(app, false));
  memset(&app->term, 0, sizeof(app->term));

exit:
  return app_err;
error:
  goto exit;
}

enum app_error
app_setup_term(struct app* app, bool enable)
{
  struct wm_device* dev = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum wm_error wm_err = WM_NO_ERROR;
  bool change_char_clbk_state = false;
  bool change_key_clbk_state = false;
  UNUSED bool b = false;

  if(!app) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  dev = app->wm.device;

  if(true == enable && false == app->term.is_enabled) {
    assert
      (  (WM(is_char_callback_attached(dev, term_char_clbk, app, &b)), !b)
      && (WM(is_key_callback_attached(dev, term_key_clbk, app, &b)), !b));
    wm_err = wm_attach_char_callback(dev, term_char_clbk, app);
    if(WM_NO_ERROR != wm_err) {
      app_err = wm_to_app_error(wm_err);
      goto error;
    }
    change_char_clbk_state = true;
    wm_err = wm_attach_key_callback(dev, term_key_clbk, app);
    if(WM_NO_ERROR != wm_err) {
      app_err = wm_to_app_error(wm_err);
      goto error;
    }
    change_key_clbk_state = true;
  } else if(false == enable && true == app->term.is_enabled) {
    assert
      (  (WM(is_char_callback_attached(dev, term_char_clbk, app, &b)), b)
      && (WM(is_key_callback_attached(dev, term_key_clbk, app, &b)), b));
    wm_err = wm_detach_char_callback(dev, term_char_clbk, app);
    if(WM_NO_ERROR != wm_err) {
      app_err = wm_to_app_error(wm_err);
      goto error;
    }
    wm_err = wm_detach_key_callback(dev, term_key_clbk, app);
    if(WM_NO_ERROR != wm_err) {
      app_err = wm_to_app_error(wm_err);
      goto error;
    }
  }
  app->term.is_enabled = enable;
exit:
  return app_err;
error:
  if(true == change_char_clbk_state) {
    if(true == enable)
      WM(detach_char_callback(dev, term_char_clbk, app));
    else
      WM(attach_char_callback(dev, term_char_clbk, app));
  }
  if(true == change_key_clbk_state) {
    if(true == enable)
      WM(detach_key_callback(dev, term_key_clbk, app));
    else
      WM(attach_key_callback(dev, term_key_clbk, app));
  }
  goto exit;
}

