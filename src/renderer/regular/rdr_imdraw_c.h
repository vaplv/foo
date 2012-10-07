#ifndef RDR_IMDRAW_C_H
#define RDR_IMDRAW_C_H

#include "renderer/rdr_error.h"
#include "renderer/rdr_imdraw.h"
#include "sys/list.h"
#include "sys/ref_count.h"
#include "sys/sys.h"

#define RDR_IMGRID_NSUBDIV 32

enum rdr_imdraw_type {
  RDR_IMDRAW_CIRCLE,
  RDR_IMDRAW_GRID,
  RDR_IMDRAW_PARALLELEPIPED,
  RDR_IMDRAW_VECTOR,
  RDR_NB_IMDRAW_TYPES
};

/* All im commands have the same size in memory. Consequently some memory space
 * is lost but this simplify the im command buffer implementation. Today, there
 * is no specific reason to create a huge command but if it become necessary
 * the memory management of the command buffer has to be adapted. */
struct rdr_imdraw_command {
  struct list_node node;
  enum rdr_imdraw_type type;
  unsigned int viewport[4]; /* { x, y, width, height } */
  int flag;
  union {
    struct rdr_im_circle {
      float transform[16]; /* column major */
      float color[4]; /* RGBA. TODO transform in RGB */
    } circle;
    struct rdr_im_vector {
      float transform[16]; /* column major */
      float vector[3];
      float color[3];/*RGB */
      enum rdr_im_vector_marker start_marker;
      enum rdr_im_vector_marker end_marker;
    } vector;
    struct rdr_im_grid {
      float transform[16]; /* column major */
      struct rdr_im_grid_desc {
        float div_color[3]; /* RGB */
        float subdiv_color[3]; /* RGB */
        float vaxis_color[3]; /* vertical axis RGB */
        float haxis_color[3]; /* horizontal axis RGB */
        unsigned int nsubdiv[2]; /* XY */
        unsigned int ndiv[2]; /* XY */
      } desc;
    } grid;
    struct rdr_im_parallelepiped {
      float transform[16]; /* column major */
      float solid_color[4]; /* RGBA */
      float wire_color[4]; /* RGBA */
    } parallelepiped;
  } data;
};

struct rdr_imdraw_command_buffer;
struct rdr_system;

/*******************************************************************************
 *
 * Im rendering system function prototypes.
 *
 ******************************************************************************/
LOCAL_SYM enum rdr_error
rdr_init_im_rendering
  (struct rdr_system* sys);

LOCAL_SYM enum rdr_error
rdr_shutdown_im_rendering
  (struct rdr_system* sys);

/*******************************************************************************
 *
 * Im draw command buffer function prototypes.
 *
 ******************************************************************************/
LOCAL_SYM enum rdr_error
rdr_create_imdraw_command_buffer
  (struct rdr_system* sys,
   size_t max_command_count,
   struct rdr_imdraw_command_buffer** out_cmdbuf);

LOCAL_SYM enum rdr_error
rdr_imdraw_command_buffer_ref_get
  (struct rdr_imdraw_command_buffer* cmdbuf);

LOCAL_SYM enum rdr_error
rdr_imdraw_command_buffer_ref_put
  (struct rdr_imdraw_command_buffer* cmdbuf);

LOCAL_SYM enum rdr_error
rdr_get_imdraw_command
  (struct rdr_imdraw_command_buffer* cmdbuf,
   struct rdr_imdraw_command** cmd);

LOCAL_SYM enum rdr_error
rdr_emit_imdraw_command
  (struct rdr_imdraw_command_buffer* cmdbuf,
   struct rdr_imdraw_command* cmd);

LOCAL_SYM enum rdr_error
rdr_flush_imdraw_command_buffer
  (struct rdr_imdraw_command_buffer* cmdbuf);

#endif /* RDR_IMDRAW_C_H */

