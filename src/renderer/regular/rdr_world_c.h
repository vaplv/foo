#ifndef RDR_WORLD_C_H
#define RDR_WORLD_C_H

#include "renderer/rdr_error.h"

struct aosf44;
struct rdr_draw_desc;
struct rdr_model_instance;
struct rdr_view;
struct rdr_world;

LOCAL_SYM enum rdr_error
rdr_draw_world
  (struct rdr_world* world,
   const struct rdr_view* view,
   const struct rdr_draw_desc* draw_desc);

LOCAL_SYM enum rdr_error
rdr_compute_projection_matrix
  (const struct rdr_view* view,
   struct aosf44* proj);

#endif /* RDR_WORLD_C_H */

