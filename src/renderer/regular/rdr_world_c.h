#ifndef RDR_WORLD_C_H
#define RDR_WORLD_C_H

#include "renderer/rdr_error.h"

struct aosf44;
struct rdr_view;
struct rdr_world;

extern enum rdr_error
rdr_draw_world
  (struct rdr_world* world,
   const struct rdr_view* view);

extern enum rdr_error
rdr_compute_projection_matrix
  (const struct rdr_view* view,
   struct aosf44* proj);

#endif /* RDR_WORLD_C_H */

