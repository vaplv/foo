#ifndef EDIT_CONTEXT_C_H
#define EDIT_CONTEXT_C_H

#include "sys/ref_count.h"
#include <stdbool.h>

/* Helper macros. */
#define EDIT_CMD_ARGVAL(argv, i) EDIT_CMD_ARGVAL_N(argv, i, 0)
#define EDIT_CMD_ARGVAL_N(argv, i, n) (argv)[(i)]->value_list[(n)]

struct app;
struct app_cvar;
struct edit_model_instance_selection;
struct mem_allocator;

enum edit_move_space {
  EDIT_MOVE_WORLD_SPACE = 0,
  EDIT_MOVE_EYE_SPACE,
  EDIT_MOVE_LOCAL_SPACE
};

enum edit_move_flag {
  EDIT_MOVE_NONE = 0,
  EDIT_MOVE_ROTATE = BIT(0),
  EDIT_MOVE_SCALE = BIT(1),
  EDIT_MOVE_TRANSLATE = BIT(2)
};

struct edit_context {
  struct ref ref;
  struct app* app;
  struct mem_allocator* allocator;
  struct edit_model_instance_selection* instance_selection;
  bool process_inputs; /* Define if the input events are intercepted */
  /* List of states of the context. */
  struct states {
    enum edit_move_space move_space;
    int move_flag;
  } states;
  /* List of client variables of the context. */
  struct cvars {
    const struct app_cvar* grid_ndiv;
    const struct app_cvar* grid_nsubdiv;
    const struct app_cvar* grid_size;
    const struct app_cvar* pivot_color;
    const struct app_cvar* pivot_size;
    const struct app_cvar* project_path;
    const struct app_cvar* show_grid;
    const struct app_cvar* show_selection;
  } cvars;
};

#endif /* EDIT_CONTEXT_C_H */

