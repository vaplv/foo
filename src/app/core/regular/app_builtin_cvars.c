#include "app/core/regular/app_builtin_cvars.h"
#include "app/core/regular/app_c.h"
#include "app/core/app_cvar.h"
#include <assert.h>

/*******************************************************************************
 *
 * Builtin cvars registration.
 *
 ******************************************************************************/
enum app_error
app_setup_builtin_cvars(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  assert(app);

  #define REGISTER_CVAR(name, desc, cvar) \
    do { \
      if((app_err = app_add_cvar(app, name, desc)) != APP_NO_ERROR) \
        goto error; \
      if((app_err = app_get_cvar(app, name, cvar)) != APP_NO_ERROR) \
        goto error; \
    } while(0)

  REGISTER_CVAR
    ("app_project_path", APP_CVAR_STRING_DESC(NULL, NULL), 
     &app->cvar_system.project_path);

  #undef CALL

exit:
  return app_err;
error:
  goto exit;
}
