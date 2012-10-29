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

struct edit_inputs_config_desc {
  float mouse_sensitivity;
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
edit_inputs_flush_commands
  (struct edit_inputs* input);

LOCAL_SYM enum edit_error
edit_inputs_config_set
  (struct edit_inputs* input,
   const struct edit_inputs_config_desc* desc);

LOCAL_SYM enum edit_error
edit_inputs_config_get
  (struct edit_inputs* input,
   struct edit_inputs_config_desc* desc);

LOCAL_SYM enum edit_error
edit_inputs_get_view_transform
  (struct edit_inputs* input,
   struct aosf44* view_transform);

#endif /* EDIT_INPUTS_H */

