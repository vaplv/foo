#include "maths/simd/aosf44.h"
#include "renderer/regular/rdr_imdraw_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/regular/rdr_term_c.h"
#include "renderer/regular/rdr_world_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_imdraw.h"
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

#define MAX_TERM_NODES 4 /* Maximum number of "draw term" call */
#define MAX_WORLD_NODES 4 /* Maximum number of draw world" call */
#define MAX_IMDRAW_COMMANDS 512
#define MAX_PICK_NODES 4

struct term_node {
  struct list_node node;
  struct rdr_term* term;
};

struct world_node {
  struct list_node node;
  struct rdr_view view;
  struct rdr_world* world;
};

struct pick_node {
  struct list_node node;
  struct rdr_view view;
  struct rdr_world* world;
  unsigned int pos[2];
  unsigned int size[2];
};

/* im draw system. */
struct imdraw {
  struct rdr_imdraw_command_buffer* cmdbuf;
  struct transform_cache {
    struct rdr_view view;
    struct aosf44 proj_matrix;
    struct aosf44 view_matrix;
    struct aosf44 viewproj_matrix;
  } transform_cache;
};

struct framebuffer {
  struct rb_framebuffer* buffer;
  struct rb_tex2d* picking_tex; /* TODO replace by a render buffer. */
  struct rb_tex2d* depth_stencil_tex; /* TODO replace bu a render buffer. */
};

struct rdr_frame {
  /* Raw memory of draw command nodes. */
  ALIGN(16) struct term_node term_node_list[MAX_TERM_NODES];
  struct world_node world_node_list[MAX_WORLD_NODES];
  struct pick_node pick_node_list[MAX_PICK_NODES];
  /* Command management. */
  size_t term_node_id;
  size_t world_node_id;
  size_t pick_node_id;
  struct list_node draw_term_list;
  struct list_node draw_world_list;
  struct list_node pick_model_instance_list;
  /* Miscellaneous */
  struct imdraw imdraw; /* im draw system. */
  struct framebuffer framebuffer;  /* offline framebuffer. */
  struct ref ref;
  struct rdr_system* sys;
  float bkg_color[4];
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static FINLINE void
compute_transform
  (struct aosf44* transform,
   const float pos[3],
   const float size[3],
   const float rotation[3])
{
  struct aosf33 f33;
  assert(transform && pos && size && rotation);

  /* Transform = translation * rotation * scale */
  aosf33_rotation(&f33, rotation[0], rotation[1], rotation[2]);
  f33.c0 = vf4_mul(f33.c0, vf4_set1(size[0]));
  f33.c1 = vf4_mul(f33.c1, vf4_set1(size[1]));
  f33.c2 = vf4_mul(f33.c2, vf4_set1(size[2]));
  aosf44_set
    (transform, f33.c0, f33.c1, f33.c2, vf4_set(pos[0], pos[1], pos[2], 1.f));
}

static void
setup_transform_cache
  (struct transform_cache* cache,
   const struct rdr_view* view)
{
  bool update_view = false;
  bool update_proj = false;
  assert(cache && view);

  update_view = 0 != memcmp
    (view->transform, cache->view.transform, sizeof(cache->view.transform));
  update_proj =
    (view->proj_ratio != cache->view.proj_ratio)
  | (view->fov_x != cache->view.fov_x)
  | (view->znear != cache->view.znear)
  | (view->zfar != cache->view.zfar);

  if(update_view | update_proj) {
    if(update_view)
      aosf44_load(&cache->view_matrix, view->transform);
    if(update_proj)
      RDR(compute_projection_matrix(view, &cache->proj_matrix));
    aosf44_mulf44
      (&cache->viewproj_matrix, &cache->proj_matrix, &cache->view_matrix);
  }
}

static void
compute_imdraw_transform
  (struct rdr_frame* frame,
   int flag,
   const struct rdr_view* view,
   const struct aosf44* model_matrix,
   float transform_matrix[16])
{
  struct aosf44 transform;
  assert
    (  frame
    && model_matrix
    && transform_matrix
    && IS_ALIGNED(transform_matrix, 16));

  setup_transform_cache(&frame->imdraw.transform_cache, view);

  if(0 == (flag & RDR_IMDRAW_FLAG_FIXED_SCREEN_SIZE)) {
    aosf44_mulf44
      (&transform,
       &frame->imdraw.transform_cache.viewproj_matrix,
       model_matrix);
  } else {
    /* We ensure a fix size of the object by scaling it with respect to its
     * distance from the eye. Following the thales' theorem the scale factor
     * `s' between the reference size `x' and the expected size `X' is equal
     * to: s = X / x = D / d
     * with `D' the real depth of the model pivot and `d' the reference depth.
     * Taking one as the reference depth leads to a scale factor equal to the
     * depth of the model pivot. it is thus sufficient to locally scale the
     * model by the depth of its pivot in view space to ensure a fix screen
     * size of the projected model. Note that since the scale factor is the
     * same for each coordinates, we just pre-multiply the model matrix by the
     * scale factor rather than pre-multiplying it by a scale matrix. */
    const vf4_t view_row2 = vf4_set
      (view->transform[2],
       view->transform[6],
       view->transform[10],
       view->transform[14]);
    const vf4_t model_pos = vf4_xyzd(model_matrix->c3, vf4_set1(1.f));
    const vf4_t model_z_vs = vf4_dot(view_row2, model_pos);
    struct aosf33 scaled_model_3x3;

    aosf33_mul
      (&scaled_model_3x3,
       (struct aosf33[]){{
          model_matrix->c0,
          model_matrix->c1,
          model_matrix->c2
       }},
       vf4_minus(model_z_vs));
    aosf44_mulf44
      (&transform,
       &frame->imdraw.transform_cache.viewproj_matrix,
       (struct aosf44[]){{
          scaled_model_3x3.c0,
          scaled_model_3x3.c1,
          scaled_model_3x3.c2,
          model_matrix->c3
       }});
  }
  aosf44_store(transform_matrix, &transform);
}

static FINLINE void
setup_imdraw_command_viewport
  (struct rdr_imdraw_command* cmd,
   const struct rdr_view* view)
{
  cmd->viewport[0] = view->x;
  cmd->viewport[1] = view->y;
  cmd->viewport[2] = view->width;
  cmd->viewport[3] = view->height;
}

static enum rdr_error
imdraw_parallelepiped
  (struct rdr_frame* frame,
   const struct rdr_view* rview,
   int flag,
   const struct aosf44* trans,
   const float solid_color[4],
   const float wire_color[4])
{
  ALIGN(16) float tmp[16];
  struct rdr_imdraw_command* cmd = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(frame && rview && trans);

  RDR(get_imdraw_command(frame->imdraw.cmdbuf, &cmd));
  if(UNLIKELY(!cmd)) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  compute_imdraw_transform(frame, flag, rview, trans, tmp);

  cmd->type = RDR_IMDRAW_PARALLELEPIPED;
  cmd->flag = flag;
  setup_imdraw_command_viewport(cmd, rview);
  memcpy(cmd->data.parallelepiped.transform, tmp, sizeof(tmp));
  if(solid_color) {
    memcpy(cmd->data.parallelepiped.solid_color, solid_color, 4*sizeof(float));
  } else {
    cmd->data.parallelepiped.solid_color[3] = 0.f;
  }
  if(wire_color) {
    memcpy(cmd->data.parallelepiped.wire_color, wire_color, 4 * sizeof(float));
  } else {
    cmd->data.parallelepiped.wire_color[3] = 0.f;
  }
  RDR(emit_imdraw_command(frame->imdraw.cmdbuf, cmd));
exit:
  return rdr_err;
error:
  assert(cmd == NULL);
  goto exit;

}

static enum rdr_error
imdraw_circle
  (struct rdr_frame* frame,
   const struct rdr_view* rview,
   int flag,
   const struct aosf44* trans,
   const float wire_color[4])
{
  ALIGN(16) float tmp[16];
  struct rdr_imdraw_command* cmd = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(frame && rview && trans && wire_color);

  RDR(get_imdraw_command(frame->imdraw.cmdbuf, &cmd));
  if(UNLIKELY(!cmd)) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }

  compute_imdraw_transform(frame, flag, rview, trans, tmp);

  cmd->type = RDR_IMDRAW_CIRCLE;
  cmd->flag = flag;
  setup_imdraw_command_viewport(cmd, rview);
  memcpy(cmd->data.circle.transform, tmp, sizeof(tmp));
  memcpy(cmd->data.circle.color, wire_color, 4 * sizeof(float));

  RDR(emit_imdraw_command(frame->imdraw.cmdbuf, cmd));
exit:
  return rdr_err;
error:
  assert(cmd == NULL);
  goto exit;
}

static enum rdr_error
imdraw_vector
  (struct rdr_frame* frame,
   const struct rdr_view* rview,
   int flag,
   const struct aosf44* trans,
   enum rdr_im_vector_marker start_marker,
   enum rdr_im_vector_marker end_marker,
   const float vec[3],
   const float color[3])
{
  ALIGN(16) float tmp[16];
  struct rdr_imdraw_command* cmd = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(frame && rview && trans && color);

  RDR(get_imdraw_command(frame->imdraw.cmdbuf, &cmd));
  if(UNLIKELY(!cmd)) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  compute_imdraw_transform(frame, flag, rview, trans, tmp);

  cmd->type = RDR_IMDRAW_VECTOR;
  cmd->flag = flag;
  setup_imdraw_command_viewport(cmd, rview);
  memcpy(cmd->data.vector.transform, tmp, sizeof(tmp));
  memcpy(cmd->data.vector.vector, vec, 3 * sizeof(float));
  memcpy(cmd->data.vector.color, color, 3 * sizeof(float));
  cmd->data.vector.start_marker = start_marker;
  cmd->data.vector.end_marker = end_marker;
  RDR(emit_imdraw_command(frame->imdraw.cmdbuf, cmd));
exit:
  return rdr_err;
error:
  assert(cmd == NULL);
  goto exit;
}

static enum rdr_error
imdraw_grid
  (struct rdr_frame* frame,
   const struct rdr_view* rview,
   int flag,
   const struct aosf44* trans,
   const unsigned int ndiv[2],
   const unsigned int nsubdiv[2],
   const float color[3],
   const float subcolor[3],
   const float vaxis_color[3],
   const float haxis_color[3])
{
  ALIGN(16) float tmp[16];
  struct rdr_imdraw_command* cmd = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(frame && rview && trans && ndiv && nsubdiv && color && subcolor &&
         vaxis_color && haxis_color);

  RDR(get_imdraw_command(frame->imdraw.cmdbuf, &cmd));
  memset(&cmd->data.grid, 0, sizeof(cmd->data.grid));
  if(UNLIKELY(!cmd)) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  compute_imdraw_transform(frame, flag, rview, trans, tmp);
  cmd->type = RDR_IMDRAW_GRID;
  setup_imdraw_command_viewport(cmd, rview);
  memcpy(cmd->data.grid.transform, tmp, sizeof(tmp));
  memcpy(cmd->data.grid.desc.ndiv, ndiv, 2 * sizeof(unsigned int));
  memcpy(cmd->data.grid.desc.nsubdiv, nsubdiv, 2 * sizeof(unsigned int));
  memcpy(cmd->data.grid.desc.div_color, color, 3 * sizeof(float));
  memcpy(cmd->data.grid.desc.subdiv_color, subcolor, 3 * sizeof(float));
  memcpy(cmd->data.grid.desc.vaxis_color, vaxis_color, 3 * sizeof(float));
  memcpy(cmd->data.grid.desc.haxis_color, haxis_color, 3 * sizeof(float));

  RDR(emit_imdraw_command(frame->imdraw.cmdbuf, cmd));
exit:
  return rdr_err;
error:
  assert(cmd == NULL);
  goto exit;
}

static void
release_framebuffer(struct rdr_system* sys, struct framebuffer* framebuffer)
{
  assert(sys && framebuffer);
  if(framebuffer->buffer)
    RBI(&sys->rb, framebuffer_ref_put(framebuffer->buffer));
  if(framebuffer->picking_tex)
    RBI(&sys->rb, tex2d_ref_put(framebuffer->picking_tex));
  if(framebuffer->depth_stencil_tex)
    RBI(&sys->rb, tex2d_ref_put(framebuffer->depth_stencil_tex));
}

static enum rdr_error
init_framebuffer
  (struct rdr_system* sys,
   struct framebuffer* framebuffer,
   const struct rdr_frame_desc* desc)
{
  struct rb_framebuffer_desc bufdesc;
  struct rb_tex2d_desc tex2ddesc;
  struct rb_render_target rt0;
  struct rb_render_target depth_stencil_rt;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  memset(&bufdesc, 0, sizeof(bufdesc));
  memset(&tex2ddesc, 0, sizeof(tex2ddesc));
  memset(&rt0, 0, sizeof(struct rb_render_target));
  memset(&depth_stencil_rt, 0, sizeof(struct rb_render_target));

  assert(framebuffer && desc);

  #define CALL(func) \
    do { \
      if((func) != 0) { \
        rdr_err = RDR_DRIVER_ERROR; \
        goto error; \
      } \
    } while(0)

  bufdesc.width = desc->width;
  bufdesc.height = desc->height;
  bufdesc.sample_count = 1;
  bufdesc.buffer_count = 1;
  CALL(sys->rb.create_framebuffer(sys->ctxt, &bufdesc, &framebuffer->buffer));

  tex2ddesc.width = desc->width;
  tex2ddesc.height = desc->height;
  tex2ddesc.mip_count = 1;
  tex2ddesc.format = RB_RG_UINT16;
  tex2ddesc.usage = RB_USAGE_DEFAULT;
  tex2ddesc.compress = 0;
  CALL(sys->rb.create_tex2d
    (sys->ctxt, &tex2ddesc, (const void*[]){NULL}, &framebuffer->picking_tex));

  tex2ddesc.format = RB_DEPTH_COMPONENT;
  CALL(sys->rb.create_tex2d
    (sys->ctxt,
     &tex2ddesc,
     (const void*[]){NULL},
     &framebuffer->depth_stencil_tex));

  rt0.type = RB_RENDER_TARGET_TEXTURE2D;
  rt0.resource = (void*)framebuffer->picking_tex;
  rt0.desc.tex2d.mip_level = 0;
  depth_stencil_rt.type = RB_RENDER_TARGET_TEXTURE2D;
  depth_stencil_rt.resource = (void*)framebuffer->depth_stencil_tex;
  depth_stencil_rt.desc.tex2d.mip_level = 0;
  CALL(sys->rb.framebuffer_render_targets
    (framebuffer->buffer, 1, &rt0, &depth_stencil_rt));
  CALL(sys->rb.framebuffer_render_targets
    (framebuffer->buffer, 1, &rt0, NULL));


  #undef CALL

exit:
  return rdr_err;
error:
  release_framebuffer(sys, framebuffer);
  memset(framebuffer, 0, sizeof(struct framebuffer));
  goto exit;
}

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
  if(frame->imdraw.cmdbuf)
    RDR(imdraw_command_buffer_ref_put(frame->imdraw.cmdbuf));
  release_framebuffer(frame->sys, &frame->framebuffer);
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
rdr_create_frame
  (struct rdr_system* sys,
   const struct rdr_frame_desc* desc,
   struct rdr_frame** out_frame)
{
  struct rdr_frame* frame = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  size_t i = 0;

  if(UNLIKELY(!sys || !desc || !out_frame)) {
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
  list_init(&frame->pick_model_instance_list);
  for(i=0; i < MAX_TERM_NODES; list_init(&frame->term_node_list[i].node), ++i);
  for(i=0; i < MAX_WORLD_NODES; list_init(&frame->world_node_list[i].node),++i);
  for(i=0; i < MAX_PICK_NODES; list_init(&frame->pick_node_list[i].node), ++i);

  rdr_err = rdr_create_imdraw_command_buffer
    (sys, MAX_IMDRAW_COMMANDS, &frame->imdraw.cmdbuf);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = init_framebuffer(frame->sys, &frame->framebuffer, desc);
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
  if(frame->world_node_id >= MAX_WORLD_NODES)
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
  if(frame->term_node_id >= MAX_TERM_NODES)
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
   int flag,
   const float pos[3],
   const float size[3],
   const float rotation[3],
   const float solid_color[4],
   const float wire_color[4])
{
  struct aosf44 transform;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(!frame || !rview || !pos || !size || !rotation)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  compute_transform(&transform, pos, size, rotation);
  rdr_err = imdraw_parallelepiped
    (frame, rview, flag, &transform, solid_color, wire_color);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  return rdr_err;
error:
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_frame_imdraw_transformed_parallelepiped
  (struct rdr_frame* frame,
   const struct rdr_view* rview,
   int flag,
   const float mat[16],
   const float solid_color[4],
   const float wire_color[4])
{
  struct aosf44 transform;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(!frame || !rview || !mat)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  if(IS_ALIGNED(mat, 16)) {
    transform.c0 = vf4_load(mat);
    transform.c1 = vf4_load(mat + 4);
    transform.c2 = vf4_load(mat + 8);
    transform.c3 = vf4_load(mat + 12);
  } else {
    transform.c0 = vf4_set(mat[0], mat[1], mat[2], mat[3]);
    transform.c1 = vf4_set(mat[4], mat[5], mat[6], mat[7]);
    transform.c2 = vf4_set(mat[8], mat[9], mat[10], mat[11]);
    transform.c3 = vf4_set(mat[12], mat[13], mat[14], mat[15]);
  }
  rdr_err = imdraw_parallelepiped
    (frame, rview, flag, &transform, solid_color, wire_color);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  return rdr_err;
error:
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_frame_imdraw_ellipse
  (struct rdr_frame* frame,
   const struct rdr_view* rview,
   int flag,
   const float pos[3],
   const float size[3],
   const float rotation[3],
   const float color[4])
{
  struct aosf44 transform;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(!frame || !rview || !pos || !size || !rotation || !color))
    return RDR_INVALID_ARGUMENT;
  compute_transform
    (&transform, pos, (float[]){size[0], size[1], 1.f}, rotation);
  rdr_err = imdraw_circle(frame, rview, flag, &transform, color);
  return rdr_err;
}

EXPORT_SYM enum rdr_error
rdr_frame_imdraw_vector
  (struct rdr_frame* frame,
   const struct rdr_view* rview,
   int flag,
   enum rdr_im_vector_marker start_marker,
   enum rdr_im_vector_marker end_marker,
   const float start[3],
   const float end[3],
   const float color[3])
{
  struct aosf44 transform;
  float vec[3] = {0.f, 0.f, 0.f};
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(!frame || !rview || !start || !end || !color))
    return RDR_INVALID_ARGUMENT;

  vec[0] = end[0] - start[0];
  vec[1] = end[1] - start[1];
  vec[2] = end[2] - start[2];
  transform.c0 = vf4_set(1.f, 0.f, 0.f, 0.f);
  transform.c1 = vf4_set(0.f, 1.f, 0.f, 0.f);
  transform.c2 = vf4_set(0.f, 0.f, 1.f, 0.f);
  transform.c3 = vf4_set(start[0], start[1], start[2], 1.f);
  rdr_err = imdraw_vector
    (frame, rview, flag, &transform, start_marker, end_marker, vec, color);
  return rdr_err;
}

EXPORT_SYM enum rdr_error
rdr_frame_imdraw_grid
  (struct rdr_frame* frame,
   const struct rdr_view* rview,
   int flag,
   const float pos[3],
   const float size[2],
   const float rotation[3],
   const unsigned int ndiv[2],
   const unsigned int nsubdiv[2],
   const float color[3],
   const float subcolor[3],
   const float vaxis_color[3],
   const float haxis_color[3])
{
  struct aosf44 transform;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY
  (  !frame
  || !rview
  || !pos
  || !size
  || !rotation
  || !ndiv
  || !nsubdiv
  || !color
  || !subcolor
  || !vaxis_color
  || !haxis_color))
    return RDR_INVALID_ARGUMENT;

  compute_transform
    (&transform, pos, (float[]){size[0], size[1], 1.f}, rotation);
  rdr_err = imdraw_grid
    (frame,
     rview,
     flag,
     &transform,
     ndiv,
     nsubdiv,
     color,
     subcolor,
     vaxis_color,
     haxis_color);
  return rdr_err;
}

EXPORT_SYM enum rdr_error
rdr_frame_pick_model_instance
  (struct rdr_frame* frame,
   struct rdr_world* world,
   const struct rdr_view* view,
   const unsigned int pos[2],
   const unsigned int size[2])
{
  struct pick_node* pick_node = NULL;

  if(UNLIKELY(!frame || !world || !view || !pos || !size))
    return RDR_INVALID_ARGUMENT;
  if(frame->pick_node_id >= MAX_PICK_NODES)
    return RDR_MEMORY_ERROR;

  pick_node = frame->pick_node_list + frame->pick_node_id;
  ++frame->pick_node_id;
  RDR(world_ref_get(world));
  pick_node->world = world;
  memcpy(&pick_node->view, view, sizeof(struct rdr_view));
  memcpy(pick_node->pos, pos, sizeof(unsigned int) * 2);
  memcpy(pick_node->size, size, sizeof(unsigned int) * 2);
  list_add(&frame->pick_model_instance_list, &pick_node->node);

  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_flush_frame(struct rdr_frame* frame)
{
  const struct rb_depth_stencil_desc depth_stencil_desc = {
    .enable_depth_write = 1,
    .enable_stencil_test = 0,
    .front_face_op.write_mask = ~0,
    .back_face_op.write_mask = ~0,
    .depth_func = RB_COMPARISON_LESS_EQUAL
  };
  const struct rb_rasterizer_desc raster_desc = {
    .fill_mode = RB_FILL_SOLID,
    .cull_mode = RB_CULL_BACK,
    .front_facing = RB_ORIENTATION_CCW
  };
  struct list_node* node = NULL;
  struct list_node* tmp = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(frame==NULL)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  RBI(&frame->sys->rb, rasterizer(frame->sys->ctxt, &raster_desc));
  RBI(&frame->sys->rb, depth_stencil(frame->sys->ctxt, &depth_stencil_desc));
  RBI(&frame->sys->rb, clear
    (frame->sys->ctxt,
     RB_CLEAR_COLOR_BIT | RB_CLEAR_DEPTH_BIT | RB_CLEAR_STENCIL_BIT,
     frame->bkg_color,
     1.f,
     0));

  /* Flush pick commands. */
  LIST_FOR_EACH_SAFE(node, tmp, &frame->pick_model_instance_list) {
    struct pick_node* pick_node = CONTAINER_OF(node, struct pick_node, node);
    /* TODO perform the picking action. */
    RDR(world_ref_put(pick_node->world));
    list_del(node);
  }
  frame->pick_node_id = 0;
  /* Flush world rendering. */
  LIST_FOR_EACH_SAFE(node, tmp, &frame->draw_world_list) {
    struct world_node* world_node = CONTAINER_OF(node, struct world_node, node);
    RDR(draw_world(world_node->world, &world_node->view));
    RDR(world_ref_put(world_node->world));
    list_del(node);
  }
  frame->world_node_id = 0;
  /* Flush im geometry rendering. */
  RDR(flush_imdraw_command_buffer(frame->imdraw.cmdbuf));
  /* Flush terminal rendering. */
  LIST_FOR_EACH_SAFE(node, tmp, &frame->draw_term_list) {
    struct term_node* term_node = CONTAINER_OF(node, struct term_node, node);
    RDR(draw_term(term_node->term));
    RDR(term_ref_put(term_node->term));
    list_del(node);
  }
  frame->term_node_id = 0;

  /* Flush render backend. */
  RBI(&frame->sys->rb, flush(frame->sys->ctxt));

exit:
  return rdr_err;
error:
  goto exit;
}

