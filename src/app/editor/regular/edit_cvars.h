#ifndef EDIT_CVARS_H
#define EDIT_CVARS_H

#include "app/editor/edit_error.h"
#include "sys/sys.h"

struct edit_context;

LOCAL_SYM enum edit_error
edit_setup_cvars
  (struct edit_context* ctxt);

LOCAL_SYM enum edit_error
edit_release_cvars
  (struct edit_context* ctxt);

#endif /* EDIT_CVARS_H. */

