#include "maths/simd/aosf44.h"
#include "renderer/regular/rdr_imdraw.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/regular/rdr_term_c.h"
#include "renderer/regular/rdr_world_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_frame.h"
#include "renderer/rdr_system.h"
#include "renderer/rdr_term.h"
#include "renderer/rdr_world.h"
#include "sys/list.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

#define MAX_TERM_NODE 4
#define MAX_WORLD_NODE 4
#define MAX_IMDRAW_COMMANDS 256

struct term_node {
  struct list_node node;
  struct rdr_term* term;
};

struct world_node {
  struct list_node node;
  struct rdr_view view;
  struct rdr_world* world;
};

struct rdr_frame {
  ALIGN(16) struct term_node term_node_list[MAX_TERM_NODE];
  struct world_node world_node_list[MAX_WORLD_NODE];
  struct ref ref;
  struct rdr_system* sys;
  struct rdr_imdraw_command_buffer* imdraw_cmdbuf;
  size_t term_node_id;
  size_t world_node_id;
  struct list_node draw_term_list;
  struct list_node draw_world_list;
  float bkg_color[4];
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_frame(struct ref* ref)
{
  struct list_node* node = NULL;
  struct rdr_frame* frame = NULL;
  struct rdr_system* sys = NULL;
  assert(ref);

  frame = CONTAINER_OF(ref, struct rdr_frame, ref);

  LIST_FOR_EACH(node, &frame->draw_term_list) {
    struct term_node* term_node = CONTAINER_OF(node, struct term_node, node);
    RDR(term_ref_put(term_node->term));
  }
  LIST_FOR_EACH(node, &frame->draw_world_list) {
    struct world_node* world_node = CONTAINER_OF(node, struct world_node, node);
    RDR(world_ref_put(world_node->world));
  }
  if(frame->imdraw_cmdbuf)
    RDR(imdraw_command_buffer_ref_put(frame->imdraw_cmdbuf));
  sys = frame->sys;
  MEM_FREE(sys->allocator, frame);
  RDR(system_ref_put(sys));
}

/*******************************************************************************
 *
 * Frame functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rdr_error
rdr_create_frame(struct rdr_system* sys, struct rdr_frame** out_frame)
{
  struct rdr_frame* frame = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  size_t i = 0;

  if(!sys || !out_frame) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  frame = MEM_ALIGNED_ALLOC(sys->allocator, sizeof(struct rdr_frame), 16);
  if(!frame) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  memset(frame, 0, sizeof(struct rdr_frame));
  ref_init(&frame->ref);
  RDR(system_ref_get(sys));
  frame->sys = sys;
  list_init(&frame->draw_world_list);
  list_init(&frame->draw_term_list);
  for(i=0; i < MAX_TERM_NODE; list_init(&frame->term_node_list[i].node), ++i);
  for(i=0; i < MAX_WORLD_NODE; list_init(&frame->world_node_list[i].node), ++i);

  rdr_err = rdr_create_imdraw_command_buffer
    (sys, MAX_IMDRAW_COMMANDS, &frame->imdraw_cmdbuf);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  if(out_frame)
    *out_frame = frame;
  return rdr_err;
error:
  if(frame) {
    RDR(frame_ref_put(frame));
    frame = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_frame_ref_get(struct rdr_frame* frame)
{
  if(!frame)
    return RDR_INVALID_ARGUMENT;
  ref_get(&frame->ref);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_frame_ref_put(struct rdr_frame* frame)
{
  if(!frame)
    return RDR_INVALID_ARGUMENT;
  ref_put(&frame->ref, release_frame);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_background_color(struct rdr_frame* frame, const float rgb[3])
{
  if(!frame || !rgb)
    return RDR_INVALID_ARGUMENT;
  memcpy(frame->bkg_color, rgb, 3 * sizeof(float));
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_frame_draw_world
  (struct rdr_frame* frame,
   struct rdr_world* world,
   const struct rdr_view* view)
{
  struct world_node* world_node = NULL;

  if(!frame || !world || !view)
    return RDR_INVALID_ARGUMENT;
  if(frame->world_node_id >= MAX_WORLD_NODE)
    return RDR_MEMORY_ERROR;

  world_node = frame->world_node_list + frame->world_node_id;
  ++frame->world_node_id;

  RDR(world_ref_get(world));
  world_node->world = world;
  memcpy(&world_node->view, view, sizeof(struct rdr_view));
  list_add(&frame->draw_world_list, &world_node->node);

  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_frame_draw_term(struct rdr_frame* frame, struct rdr_term* term)
{
  struct term_node* term_node = NULL;

  if(!frame || !term)
    return RDR_INVALID_ARGUMENT;
  if(frame->term_node_id >= MAX_TERM_NODE)
    return RDR_MEMORY_ERROR;

  term_node = frame->term_node_list + frame->term_node_id;
  ++frame->term_node_id;
  RDR(term_ref_get(term));
  term_node->term = term;
  list_add(&frame->draw_term_list, &term_node->node);

  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_frame_imdraw_parallelepiped
  (struct rdr_frame* frame,
   const struct rdr_view* rview,
   float pos[3],
   float size[3],
   float rotation[3],
   float solid_color[4],
   float wire_color[4])
{
  ALIGN(16) float tmp[16];
  struct aosf44 viewproj;
  struct aosf44 proj;
  struct aosf44 view;
  struct aosf44 transform;
  struct aosf33 f33;
  struct rdr_imdraw_command* cmd = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(!frame || !rview || !pos || !size || !rotation)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  RDR(get_imdraw_command(frame->imdraw_cmdbuf, &cmd));
  if(UNLIKELY(!cmd)) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  /* Compute the transform matrix. */
  aosf44_load(&view, rview->transform);
  RDR(compute_projection_matrix(rview, &proj));
  aosf33_rotation(&f33, rotation[0], rotation[1], rotation[2]);
  f33.c0 = vf4_mul(f33.c0, vf4_set1(size[0]));
  f33.c1 = vf4_mul(f33.c1, vf4_set1(size[1]));
  f33.c2 = vf4_mul(f33.c1, vf4_set1(size[2]));
  aosf44_set
    (&transform, f33.c0, f33.c1, f33.c2, vf4_set(pos[0], pos[1], pos[2], 1.f));
  aosf44_mulf44(&viewproj, &proj, &view);
  aosf44_mulf44(&transform, &viewproj, &transform);
  aosf44_store(tmp, &transform);
  /* Emit the draw command. */
  cmd->type = RDR_IMDRAW_PARALLELEPIPED;
  memcpy(cmd->data.parallelepiped.transform, tmp, sizeof(tmp));
  memcpy(cmd->data.parallelepiped.solid_color, solid_color, 4 * sizeof(float));
  memcpy(cmd->data.parallelepiped.wire_color, wire_color, 4 * sizeof(float));
  RDR(emit_imdraw_command(frame->imdraw_cmdbuf, cmd));
exit:
  return rdr_err;
error:
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_flush_frame(struct rdr_frame* frame)
{
  struct list_node* node = NULL;
  struct list_node* tmp = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(frame==NULL)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  RBI(&frame->sys->rb, clear
    (frame->sys->ctxt,
     RB_CLEAR_COLOR_BIT | RB_CLEAR_DEPTH_BIT | RB_CLEAR_STENCIL_BIT,
     frame->bkg_color,
     1.f,
     0));
  LIST_FOR_EACH_SAFE(node, tmp, &frame->draw_world_list) {
    struct world_node* world_node = CONTAINER_OF(node, struct world_node, node);
    RDR(draw_world(world_node->world, &world_node->view));
    RDR(world_ref_put(world_node->world));
    list_del(node);
  }
  LIST_FOR_EACH_SAFE(node, tmp, &frame->draw_term_list) {
    struct term_node* term_node = CONTAINER_OF(node, struct term_node, node);
    RDR(draw_term(term_node->term));
    RDR(term_ref_put(term_node->term));
    list_del(node);
  }
  frame->world_node_id = 0;
  frame->term_node_id = 0;
  RBI(&frame->sys->rb, flush(frame->sys->ctxt));

exit:
  return rdr_err;
error:
  goto exit;
}

