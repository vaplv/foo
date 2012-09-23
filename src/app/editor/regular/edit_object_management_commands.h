#ifndef EDIT_OBJECT_MANAGEMENT_COMMANDS_H
#define EDIT_OBJECT_MANAGEMENT_COMMANDS_H

#include "app/editor/edit_error.h"
#include "sys/sys.h"

struct edit_context;

LOCAL_SYM enum edit_error
edit_setup_object_management_commands
  (struct edit_context* ctxt);

LOCAL_SYM enum edit_error
edit_release_object_management_commands
  (struct edit_context* ctxt);

#endif /* EDIT_OBJECT_MANAGEMENT_COMMANDS_H */

