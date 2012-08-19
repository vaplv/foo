#include "app/core/app.h"
#include "app/core/app_cvar.h"
#include "app/editor/regular/edit_context_c.h"
#include "app/editor/regular/edit_cvars.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/edit_context.h"
#include <float.h>
#include <string.h>

/*******************************************************************************
 *
 * Builtin cvars registration.
 *
 ******************************************************************************/
enum edit_error
edit_setup_cvars(struct edit_context* ctxt)
{
  enum app_error app_err = APP_NO_ERROR;
  enum edit_error edit_err = EDIT_NO_ERROR;

  if(UNLIKELY(!ctxt)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  #define EDIT_CVAR(name, desc) \
    app_err = app_add_cvar(ctxt->app, STR(CONCAT(edit_, name)), desc); \
    if(app_err != APP_NO_ERROR) { \
      edit_err = app_to_edit_error(app_err);\
      goto error;\
    } \
    app_err = app_get_cvar \
      (ctxt->app, STR(CONCAT(edit_, name)), &ctxt->cvars.name);  \
    if(app_err != APP_NO_ERROR) { \
      edit_err = app_to_edit_error(app_err); \
      goto error; \
    }

  #include "app/editor/regular/edit_cvars_decl.h"

  #undef EDIT_CVAR

exit:
  return edit_err;
error:
  if(ctxt)
    EDIT(release_cvars(ctxt));
  goto exit;
}

enum edit_error
edit_release_cvars(struct edit_context* ctxt)
{
  const struct app_cvar* cvar = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;

  if(UNLIKELY(!ctxt)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  #define EDIT_CVAR(name, desc) \
    APP(get_cvar(ctxt->app, STR(CONCAT(edit_, name)), &cvar)); \
    if(cvar != NULL) { \
      APP(del_cvar(ctxt->app, STR(CONCAT(edit_, name)))); \
    }

  #include "app/editor/regular/edit_cvars_decl.h"

  #undef EDIT_CVAR

  memset(&ctxt->cvars, 0, sizeof(ctxt->cvars));

exit:
  return edit_err;
error:
  goto exit;
}

