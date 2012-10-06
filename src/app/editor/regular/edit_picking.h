#ifndef EDIT_PICKING_H
#define EDIT_PICKING_H

#include "app/editor/edit_error.h"
#include "sys/sys.h"

struct edit_context;

LOCAL_SYM enum edit_error
edit_process_picking
  (struct edit_context* ctxt);

#endif /* EDIT_PICKING_H */

