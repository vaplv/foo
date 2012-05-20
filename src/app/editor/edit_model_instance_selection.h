#ifndef EDIT_MODEL_INSTANCE_SELECTION_H
#define EDIT_MODEL_INSTANCE_SELECTION_H

#include "app/editor/edit_error.h"
#include <stdbool.h>

struct app_model_instance;
struct edit_context;
struct edit_model_instance_selection;

extern enum edit_error
edit_create_model_instance_selection
  (struct edit_context* ctxt,
   struct edit_model_instance_selection** selection);

extern enum edit_error
edit_model_instance_selection_ref_get
  (struct edit_model_instance_selection* selection);

extern enum edit_error
edit_model_instance_selection_ref_put
  (struct edit_model_instance_selection* selection);

extern enum edit_error
edit_select_model_instance
  (struct edit_model_instance_selection* selection,
   struct app_model_instance* instance);

extern enum edit_error
edit_unselect_model_instance
  (struct edit_model_instance_selection* selection,
   struct app_model_instance* instance);

extern enum edit_error
edit_is_model_instance_selected
  (struct edit_model_instance_selection* selection,
   struct app_model_instance* instance,
   bool* is_selected);

extern enum edit_error
edit_remove_selected_model_instances
  (struct edit_model_instance_selection* selection);

extern enum edit_error
edit_clear_model_instance_selection
  (struct edit_model_instance_selection* selection);

extern enum edit_error
edit_get_model_instance_selection_pivot
  (struct edit_model_instance_selection* selection,
   float pivot[3]);

extern enum edit_error
edit_translate_model_instance_selection
  (struct edit_model_instance_selection* selection,
   bool local_translation,
   const float translation[3]);

extern enum edit_error
edit_draw_model_instance_selection
  (struct edit_model_instance_selection* selection);

#endif /* EDIT_MODEL_INSTANCE_SELECTION_H */

