#ifndef EDIT_MODEL_INSTANCE_SELECTION_H
#define EDIT_MODEL_INSTANCE_SELECTION_H

#include "app/editor/edit_error.h"
#include <stdbool.h>

struct edit_context;

extern enum edit_error
edit_init_model_instance_selection
  (struct edit_context* ctxt);

extern enum edit_error
edit_shutdown_model_instance_selection
  (struct edit_context* ctxt);

extern enum edit_error
edit_select_model_instance
  (struct edit_context* ctxt,
   const char* instance_name);

extern enum edit_error
edit_unselect_model_instance
  (struct edit_context* ctxt,
   const char* instance_name);

extern enum edit_error
edit_is_model_instance_selected
  (struct edit_context* ctxt,
   const char* instance_name,
   bool* is_selected);

extern enum edit_error
edit_clear_model_instance_selection
  (struct edit_context* ctxt);

extern enum edit_error
edit_get_model_instance_selection_pivot
  (struct edit_context* ctxt,
   float pivot[3]); 

extern enum edit_error
edit_translate_model_instance_selection
  (struct edit_context* ctxt,
   bool local_translation,
   const float translation[3]);

extern enum edit_error
edit_draw_model_instance_selection
  (struct edit_context* ctxt);

#endif /* EDIT_MODEL_INSTANCE_SELECTION_H. */

