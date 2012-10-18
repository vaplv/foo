#ifndef RDR_PICKING_H
#define RDR_PICKING_H

#include "renderer/rdr_error.h"
#include "sys/sys.h"

struct rdr_imdraw_command_buffer;
struct rdr_model_instance;
struct rdr_picking;
struct rdr_system;
struct rdr_view;
struct rdr_world;

struct rdr_picking_desc {
  unsigned int width, height; /* definition in pixels */
};

LOCAL_SYM enum rdr_error
rdr_create_picking
  (struct rdr_system* sys,
   const struct rdr_picking_desc* desc,
   struct rdr_picking** picking);

LOCAL_SYM enum rdr_error
rdr_free_picking
  (struct rdr_system* sys,
   struct rdr_picking* picking);

LOCAL_SYM enum rdr_error
rdr_pick
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   struct rdr_world* world,
   const struct rdr_view* view,
   const unsigned int pos[2], /* in screen space pixels */
   const unsigned int size[2]); /* in screen space pixels */

/* Pick the im geometries actually enqueued into cmdbuf. */
LOCAL_SYM enum rdr_error
rdr_pick_imdraw
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   struct rdr_imdraw_command_buffer* cmdbuf,
   const unsigned int pos[2], /* in screen space pixels */
   const unsigned int size[2]); /* in screen space pixels */

/* Retrieve current picked id. */
LOCAL_SYM enum rdr_error
rdr_pick_poll
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   size_t* count,
   const uint32_t* out_list[]);

/* Draw the pick buffer into the default framebuffer */
LOCAL_SYM enum rdr_error
rdr_show_pick_buffer
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   struct rdr_world* world,
   const struct rdr_view* view,
   const uint32_t max_pick_id);

#endif /* RDR_PICKING_H */

