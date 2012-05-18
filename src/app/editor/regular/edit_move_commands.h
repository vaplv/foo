#ifndef EDIT_MOVE_COMMANDS_H
#define EDIT_MOVE_COMMANDS_H

#include "app/editor/edit_error.h"

struct edit_context;

extern enum edit_error
edit_setup_move_commands
  (struct edit_context* ctxt);

extern enum edit_error
edit_release_move_commands
  (struct edit_context* ctxt);

#endif /* EDIT_MOVE_COMMANDS_H */
