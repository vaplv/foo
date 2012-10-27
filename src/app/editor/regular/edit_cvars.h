#ifndef EDIT_CVARS_H
#define EDIT_CVARS_H

#include "app/editor/edit_error.h"
#include "sys/sys.h"

struct app;
struct edit_cvars;

/* List of client variables of the context. */
struct edit_cvars {
  #define EDIT_CVAR(name, desc) const struct app_cvar* name;
  #include "app/editor/regular/edit_cvars_decl.h"
  #undef EDIT_CVAR
};

LOCAL_SYM enum edit_error
edit_setup_cvars
  (struct app* app,
   struct edit_cvars* cvars);

LOCAL_SYM enum edit_error
edit_release_cvars
  (struct app* app,
   struct edit_cvars* cvars);

#endif /* EDIT_CVARS_H. */

