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
app_setup_builtin_cvars(struct app* app UNUSED)
{
  assert(app);
  /* Right now there is no builtin cvars. */
  return APP_NO_ERROR;
}
