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
  enum edit_error edit_err = EDIT_NO_ERROR;

  if(UNLIKELY(!ctxt)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  #define REGISTER_CVAR(name, desc, cvar) \
    do { \
      enum app_error app_err = APP_NO_ERROR; \
      if((app_err = app_add_cvar(ctxt->app, name, desc)) != APP_NO_ERROR) { \
        edit_err = app_to_edit_error(app_err); \
        goto error; \
      } \
      if((app_err = app_get_cvar(ctxt->app, name, cvar)) != APP_NO_ERROR) { \
        edit_err = app_to_edit_error(app_err); \
        goto error; \
      } \
    } while(0)

  REGISTER_CVAR
    ("edit_project_path", APP_CVAR_STRING_DESC(NULL, NULL),
     &ctxt->cvars.project_path);
  REGISTER_CVAR
    ("edit_show_selection", APP_CVAR_BOOL_DESC(true),
     &ctxt->cvars.show_selection);
  REGISTER_CVAR
    ("edit_pivot_size", APP_CVAR_FLOAT_DESC(0.1f, FLT_MIN, FLT_MAX),
     &ctxt->cvars.pivot_size);

  #undef REGISTER_CVAR

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
  enum edit_error edit_err = EDIT_NO_ERROR;

  if(UNLIKELY(!ctxt)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  #define UNREGISTER_CVAR(name) \
    do { \
      const struct app_cvar* cvar = NULL; \
      APP(get_cvar(ctxt->app, name, &cvar)); \
      if(cvar != NULL) { \
        APP(del_cvar(ctxt->app, name)); \
      } \
    } while(0)

  UNREGISTER_CVAR("edit_project_path");
  UNREGISTER_CVAR("edit_show_selection");
  UNREGISTER_CVAR("edit_pivot_size");

  #undef UNREGISTER_CVAR

  memset(&ctxt->cvars, 0, sizeof(ctxt->cvars));

exit:
  return edit_err;
error:
  goto exit;
}

