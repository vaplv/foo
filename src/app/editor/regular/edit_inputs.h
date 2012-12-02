#ifndef EDIT_INPUTS_H
#define EDIT_INPUTS_H

#include "app/editor/edit_error.h"
#include "maths/simd/aosf44.h"
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

enum edit_selection_mode {
  EDIT_SELECTION_MODE_NEW,
  EDIT_SELECTION_MODE_NONE,
  EDIT_SELECTION_MODE_XOR
};

struct edit_inputs_state {
  struct aosf44 view_transform;
  enum edit_selection_mode selection_mode;
  int entity_transform_flag; /* Combination of edit_transform_flag */
  bool is_enabled;
};

struct aosf44;
struct edit_inputs;
struct mem_allocator;

LOCAL_SYM enum edit_error
edit_create_inputs
  (struct app* app,
   struct mem_allocator* allocator,
   struct edit_inputs** input);

LOCAL_SYM enum edit_error
edit_inputs_ref_get
  (struct edit_inputs* input);

LOCAL_SYM enum edit_error
edit_inputs_ref_put
  (struct edit_inputs* input);

LOCAL_SYM enum edit_error
edit_inputs_enable
  (struct edit_inputs* input);
 
LOCAL_SYM enum edit_error
edit_inputs_disable
  (struct edit_inputs* input);
 
LOCAL_SYM enum edit_error
edit_inputs_is_enabled
  (struct edit_inputs* input,
   bool* is_enabled);

LOCAL_SYM enum edit_error
edit_inputs_flush_commands
  (struct edit_inputs* input);

LOCAL_SYM enum edit_error
edit_inputs_set_mouse_sensitivity
  (struct edit_inputs* input,
   const float sensitivity);

LOCAL_SYM enum edit_error
edit_inputs_flush
  (struct edit_inputs* input);

LOCAL_SYM enum edit_error
edit_inputs_get_state
  (const struct edit_inputs* input,
   struct edit_inputs_state* state);
 
#endif /* EDIT_INPUTS_H */

