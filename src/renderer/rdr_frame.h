#ifndef RDR_FRAME_H
#define RDR_FRAME_H

#include "renderer/rdr.h"
#include "renderer/rdr_error.h"
#include "renderer/rdr_imdraw.h"
#include <stdbool.h>

struct rdr_frame;
struct rdr_model_instance;
struct rdr_system;
struct rdr_term;
struct rdr_view;
struct rdr_world;

struct rdr_frame_desc {
  unsigned int width;
  unsigned int height;
};

RDR_API enum rdr_error
rdr_create_frame
  (struct rdr_system* sys,
   const struct rdr_frame_desc* desc,
   struct rdr_frame** frame);

RDR_API enum rdr_error
rdr_frame_ref_get
  (struct rdr_frame* frame);

RDR_API enum rdr_error
rdr_frame_ref_put
  (struct rdr_frame* frame);

RDR_API enum rdr_error
rdr_background_color
  (struct rdr_frame* frame,
   const float rgb[3]);

RDR_API enum rdr_error
rdr_flush_frame
  (struct rdr_frame* frame);

/*******************************************************************************
 *
 * Draw commands.
 *
 ******************************************************************************/
RDR_API enum rdr_error
rdr_frame_draw_world
  (struct rdr_frame* frame,
   struct rdr_world* world,
   const struct rdr_view* view);

RDR_API enum rdr_error
rdr_frame_draw_term
  (struct rdr_frame* frame,
   struct rdr_term* term);

/*******************************************************************************
 *
 * Im draw commands.
 *
 ******************************************************************************/
RDR_API enum rdr_error
rdr_frame_imdraw_parallelepiped
  (struct rdr_frame* frame,
   const struct rdr_view* view,
   int flag,
   const float pos[3],
   const float size[3],
   const float rotation[3],
   const float solid_color[4], /* May be NULL <=> No solid parallelepiped. */
   const float wire_color[4]); /* May be NULL <=> No wire parallelepiped. */

RDR_API enum rdr_error
rdr_frame_imdraw_transformed_parallelepiped
  (struct rdr_frame* frame,
   const struct rdr_view* view,
   int flag,
   const float transform[16],
   const float solid_color[4], /* May be NULL <=> No solid parallelepiped. */
   const float wire_color[4]); /* May be NULL <=> No wire parallelepiped. */

RDR_API enum rdr_error
rdr_frame_imdraw_ellipse
  (struct rdr_frame* frame,
   const struct rdr_view* view,
   int flag,
   const float pos[3],
   const float size[2],
   const float rotation[3],
   const float color[4]);

RDR_API enum rdr_error
rdr_frame_imdraw_grid
  (struct rdr_frame* frame,
   const struct rdr_view* view,
   int flag,
   const float pos[3],
   const float size[2],
   const float rotation[3],
   const unsigned int ndiv[2],
   const unsigned int nsubdiv[2],
   const float color[3],
   const float subcolor[3],
   const float vaxis_color[3],
   const float haxis_color[3]);

RDR_API enum rdr_error
rdr_frame_imdraw_vector
  (struct rdr_frame* frame,
   const struct rdr_view* view,
   int flag,
   enum rdr_im_vector_marker start_marker,
   enum rdr_im_vector_marker end_marker,
   const float start[3],
   const float end[3],
   const float color[3]);

/*******************************************************************************
 *
 * Pick commands.
 *
 ******************************************************************************/
RDR_API enum rdr_error
rdr_frame_pick_model_instance
  (struct rdr_frame* frame,
   struct rdr_world* world,
   const struct rdr_view* view,
   const unsigned int pos[2], /* In pixels. */
   const unsigned int size[2]); /* In pixels. */

RDR_API enum rdr_error
rdr_frame_get_picked_ids
  (struct rdr_frame* frame,
   size_t* count,
   const uint32_t* picked_ids_list[]);

RDR_API enum rdr_error
rdr_frame_show_pick_buffer
  (struct rdr_frame* frame,
   struct rdr_world* world,
   const struct rdr_view* view);

#endif /* RDR_FRAME_H */

