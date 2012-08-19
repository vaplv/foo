#ifndef EDIT_CONTEXT_C_H
#define EDIT_CONTEXT_C_H

#include "maths/simd/aosf44.h"
#include "sys/ref_count.h"
#include <stdbool.h>

/* Helper macros. */
#define EDIT_CMD_ARGVAL(argv, i) EDIT_CMD_ARGVAL_N(argv, i, 0)
#define EDIT_CMD_ARGVAL_N(argv, i, n) (argv)[(i)]->value_list[(n)]

struct app;
struct app_cvar;
struct edit_model_instance_selection;
struct mem_allocator;

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

ALIGN(16) struct edit_context {
  struct view {
    struct aosf44 transform;
  } view;
  struct ref ref;
  struct app* app;
  struct mem_allocator* allocator;
  struct edit_model_instance_selection* instance_selection;
  bool process_inputs; /* Define if the input events are intercepted */
  /* List of states of the context. */
  struct states {
    enum edit_transform_space transform_space;
    int entity_transform_flag;
    int view_transform_flag;
    int mouse_cursor[2];
    int mouse_wheel;
  } states;
  struct inputs {
    float view_translation[3];
    float view_rotation[3];
    bool updated;
  } inputs;
  /* List of client variables of the context. */
  struct cvars {
    #define EDIT_CVAR(name, desc) const struct app_cvar* name;
    #include "app/editor/regular/edit_cvars_decl.h"
    #undef EDIT_CVAR
  } cvars;
};


#endif /* EDIT_CONTEXT_C_H */

