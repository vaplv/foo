#ifndef EDIT_MODEL_INSTANCE_SELECTION_C_H
#define EDIT_MODEL_INSTANCE_SELECTION_C_H

#include "app/editor/edit_error.h"
#include "sys/sys.h"

struct edit_context;
struct edit_model_instance_selection;

/* Ensure that the created selection does not take the ownership of the edit
 * context. Usefull for the builtin selection. */
LOCAL_SYM enum edit_error
edit_regular_create_model_instance_selection
  (struct edit_context* ctxt,
   struct edit_model_instance_selection** selection);

/* Does not release the ownership of the edit context. */
LOCAL_SYM enum edit_error
edit_regular_model_instance_selection_ref_put
  (struct edit_model_instance_selection* selection);

#endif /* EDIT_MODEL_INSTANCE_SELECTION_C_H */

