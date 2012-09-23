#ifndef EDIT_INPUTS_H
#define EDIT_INPUTS_H

#include "app/editor/edit_error.h"
#include "sys/sys.h"

struct edit_context;

LOCAL_SYM enum edit_error
edit_init_inputs
  (struct edit_context* ctxt);

LOCAL_SYM enum edit_error
edit_release_inputs
  (struct edit_context* ctxt);

LOCAL_SYM enum edit_error
edit_process_inputs
  (struct edit_context* ctxt);

#endif /* EDIT_INPUTS_H */

