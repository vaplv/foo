#ifndef RDR_WORLD_C_H
#define RDR_WORLD_C_H

#include "renderer/rdr_error.h"

struct aosf44;
struct rdr_draw_desc;
struct rdr_model_instance;
struct rdr_view;
struct rdr_world;

extern enum rdr_error
rdr_draw_world
  (struct rdr_world* world,
   const struct rdr_view* view,
   const struct rdr_draw_desc* draw_desc);

extern enum rdr_error
rdr_compute_projection_matrix
  (const struct rdr_view* view,
   struct aosf44* proj);

/* Returned the list of instances registered against the world.
 * The returned instances can be indexed by the draw id. */
extern enum rdr_error
rdr_get_world_model_instance_list
  (struct rdr_world* world,
   size_t* size, /* Maye be NULL. */
   struct rdr_model_instance** instance_list[]); /* May be NULL. */
                              

#endif /* RDR_WORLD_C_H */

