#ifndef EDIT_PICKING_H
#define EDIT_PICKING_H

#include "app/editor/edit_error.h"
#include "sys/sys.h"

struct edit_inputs;
struct edit_model_instance_selection;
struct edit_picking;

LOCAL_SYM enum edit_error
edit_create_picking
  (struct app* app,
   struct edit_inputs* inputs,
   struct edit_model_instance_selection* instance_selection,
   struct mem_allocator* allocator,
   struct edit_picking** picking);

LOCAL_SYM enum edit_error
edit_picking_ref_get
  (struct edit_picking* picking);

LOCAL_SYM enum edit_error
edit_picking_ref_put
  (struct edit_picking* picking);

LOCAL_SYM enum edit_error
edit_process_picking
  (struct edit_picking* ctxt);

#endif /* EDIT_PICKING_H */

