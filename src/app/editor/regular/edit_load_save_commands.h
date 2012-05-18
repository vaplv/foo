#ifndef EDIT_LOAD_SAVE_COMMANDS_H
#define EDIT_LOAD_SAVE_COMMANDS_H

#include "app/editor/edit_error.h"

struct edit_context;

extern enum edit_error
edit_setup_load_save_commands
  (struct edit_context* ctxt);

extern enum edit_error
edit_release_load_save_commands
  (struct edit_context* ctxt);

#endif /* EDIT_LOAD_SAVE_COMMANDS_H */

