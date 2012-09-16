#ifndef RDR_PICKING_H
#define RDR_PICKING_H

#include "renderer/rdr_error.h"
#include "sys/sys.h"

struct rdr_picking;
struct rdr_system;
struct rdr_view;
struct rdr_world;

struct rdr_picking_desc {
  unsigned int width, height; /* definition in pixels */
};

enum rdr_pick {
  RDR_PICK_MODEL_INSTANCE,
  RDR_PICK_NONE
};

extern enum rdr_error
rdr_create_picking
  (struct rdr_system* sys,
   const struct rdr_picking_desc* desc,
   struct rdr_picking** picking);

extern enum rdr_error
rdr_free_picking
  (struct rdr_system* sys,
   struct rdr_picking* picking);

extern enum rdr_error
rdr_pick
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   struct rdr_world* world,
   const struct rdr_view* view,
   const unsigned int pos[2], /* in screen space pixels. */
   const unsigned int size[2], /* in screen space pixels. */
   enum rdr_pick pick_type);

/* Draw the pick buffer into the default framebuffer. */
extern enum rdr_error
rdr_show_pick_buffer
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   struct rdr_world* world,
   const struct rdr_view* view,
   enum rdr_pick pick_type);


#endif /* RDR_PICKING_H */

