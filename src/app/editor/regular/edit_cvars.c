#include "app/core/app.h"
#include "app/core/app_cvar.h"
#include "app/editor/regular/edit_cvars.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/edit.h"
#include <float.h>
#include <string.h>

/*******************************************************************************
 *
 * Builtin cvars registration.
 *
 ******************************************************************************/
enum edit_error
edit_setup_cvars(struct app* app, struct edit_cvars* cvars)
{
  enum app_error app_err = APP_NO_ERROR;
  enum edit_error edit_err = EDIT_NO_ERROR;

  if(UNLIKELY(!app || !cvars)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  #define EDIT_CVAR(name, desc) \
    app_err = app_add_cvar(app, STR(CONCAT(edit_, name)), desc); \
    if(app_err != APP_NO_ERROR) { \
      edit_err = app_to_edit_error(app_err);\
      goto error;\
    } \
    app_err = app_get_cvar(app, STR(CONCAT(edit_, name)), &cvars->name); \
    if(app_err != APP_NO_ERROR) { \
      edit_err = app_to_edit_error(app_err); \
      goto error; \
    }
  #include "app/editor/regular/edit_cvars_decl.h"
  #undef EDIT_CVAR

exit:
  return edit_err;
error:
  if(app && cvars)
    EDIT(release_cvars(app, cvars));
  goto exit;
}

enum edit_error
edit_release_cvars(struct app* app, struct edit_cvars* cvars)
{
  const struct app_cvar* cvar = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;

  if(UNLIKELY(!app || !cvars)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  #define EDIT_CVAR(name, desc) \
    APP(get_cvar(app, STR(CONCAT(edit_, name)), &cvar)); \
    if(cvar != NULL) { \
      APP(del_cvar(app, STR(CONCAT(edit_, name)))); \
    }
  #include "app/editor/regular/edit_cvars_decl.h"
  #undef EDIT_CVAR

  memset(cvars, 0, sizeof(struct edit_cvars));

exit:
  return edit_err;
error:
  goto exit;
}

