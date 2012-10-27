#ifndef EDIT_INPUTS_H
#define EDIT_INPUTS_H

#include "app/editor/edit_error.h"
#include "sys/sys.h"
#include <stdbool.h>

enum edit_transform_space {
  EDIT_TRANSFORM_WORLD_SPACE = 0,
  EDIT_TRANSFORM_EYE_SPACE,
  EDIT_TRANSFORM_LOCAL_SPACE
};

enum edit_transform_flag {
  EDIT_TRANSFORM_NONE = 0,
  EDIT_TRANSFORM_ROTATE = BIT(0),
  EDIT_TRANSFORM_SCALE = BIT(1),
  EDIT_TRANSFORM_TRANSLATE = BIT(2)
};

/* List of states of the context */
struct edit_states {
  enum edit_transform_space transform_space;
  int entity_transform_flag; /* combination of edit_transform_flag */
  int view_transform_flag; /* combination of edit_transform_flag */
  int mouse_cursor[2];
  int mouse_wheel;
  bool in_selection; /* Define if selection mode is enabled */
};

struct edit_inputs {
  float view_translation[3];
  float view_rotation[3];
  bool updated;
};

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

