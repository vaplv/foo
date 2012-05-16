#ifndef RDR_FRAME_H
#define RDR_FRAME_H

#include "renderer/rdr_error.h"

struct rdr_frame;
struct rdr_system;
struct rdr_term;
struct rdr_view;
struct rdr_world;

extern enum rdr_error
rdr_create_frame
  (struct rdr_system* sys,
   struct rdr_frame** frame);

extern enum rdr_error
rdr_frame_ref_get
  (struct rdr_frame* frame);

extern enum rdr_error
rdr_frame_ref_put
  (struct rdr_frame* frame);

extern enum rdr_error
rdr_background_color
  (struct rdr_frame* frame,
   const float rgb[3]);

extern enum rdr_error
rdr_frame_draw_world
  (struct rdr_frame* frame,
   struct rdr_world* world,
   const struct rdr_view* view);

extern enum rdr_error
rdr_frame_draw_term
  (struct rdr_frame* frame,
   struct rdr_term* term);

extern enum rdr_error
rdr_frame_imdraw_parallelepiped
  (struct rdr_frame* frame,
   const struct rdr_view* view,
   const float pos[3],
   const float size[3],
   const float rotation[3],
   const float solid_color[4], /* May be NULL <=> No solid parallelepiped. */
   const float wire_color[4]); /* May be NULL <=> No wire parallelepiped. */

extern enum rdr_error
rdr_frame_imdraw_transformed_parallelepiped
  (struct rdr_frame* frame,
   const struct rdr_view* view,
   const float transform[16],
   const float solid_color[4], /* May be NULL <=> No solid parallelepiped. */
   const float wire_color[4]); /* May be NULL <=> No wire parallelepiped. */

extern enum rdr_error
rdr_flush_frame
  (struct rdr_frame* frame);

#endif /* RDR_FRAME_H */

