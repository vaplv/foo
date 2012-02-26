#ifndef RDR_WORLD_C_H
#define RDR_WORLD_C_H

#include "renderer/rdr_error.h"

struct rdr_view;
struct rdr_world;

extern enum rdr_error
rdr_draw_world
  (struct rdr_world* world,
   const struct rdr_view* view);

#endif /* RDR_WORLD_C_H */

