#ifndef EDIT_INPUTS_H
#define EDIT_INPUTS_H

#include "app/editor/edit_error.h"

struct edit_context;

extern enum edit_error
edit_init_inputs
  (struct edit_context* ctxt);

extern enum edit_error
edit_release_inputs
  (struct edit_context* ctxt);

#endif /* EDIT_INPUTS_H */

