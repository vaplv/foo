#ifndef RDR_IMDRAW_C_H
#define RDR_IMDRAW_C_H

#include "renderer/rdr_error.h"
#include "sys/list.h"
#include "sys/ref_count.h"

enum rdr_imdraw_type {
  RDR_IMDRAW_CIRCLE,
  RDR_IMDRAW_PARALLELEPIPED,
  RDR_NB_IMDRAW_TYPES
};

struct rdr_imdraw_command {
  struct list_node node;
  enum rdr_imdraw_type type;
  unsigned int viewport[4]; /* { x, y, width, height } */
  int flag;
  union {
    struct {
      float transform[16]; /* column major */
      float color[4]; /* RGBA */
    } circle;
    struct {
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
extern enum rdr_error
rdr_init_im_rendering
  (struct rdr_system* sys);

extern enum rdr_error
rdr_shutdown_im_rendering
  (struct rdr_system* sys);

/*******************************************************************************
 *
 * Im draw command buffer function prototypes.
 *
 ******************************************************************************/
extern enum rdr_error
rdr_create_imdraw_command_buffer
  (struct rdr_system* sys,
   size_t max_command_count,
   struct rdr_imdraw_command_buffer** out_cmdbuf);

extern enum rdr_error
rdr_imdraw_command_buffer_ref_get
  (struct rdr_imdraw_command_buffer* cmdbuf);

extern enum rdr_error
rdr_imdraw_command_buffer_ref_put
  (struct rdr_imdraw_command_buffer* cmdbuf);

extern enum rdr_error
rdr_get_imdraw_command
  (struct rdr_imdraw_command_buffer* cmdbuf,
   struct rdr_imdraw_command** cmd);

extern enum rdr_error
rdr_emit_imdraw_command
  (struct rdr_imdraw_command_buffer* cmdbuf,
   struct rdr_imdraw_command* cmd);

extern enum rdr_error
rdr_flush_imdraw_command_buffer
  (struct rdr_imdraw_command_buffer* cmdbuf);

#endif /* RDR_IMDRAW_C_H */

