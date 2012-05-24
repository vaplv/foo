#include "renderer/regular/rdr_imdraw.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_system.h"
#include "sys/mem_allocator.h"
#include <string.h>

struct rdr_imdraw_command_buffer {
  struct ref ref;
  struct rdr_system* sys;
  size_t max_command_count;
  struct list_node emit_command_list; /* Emitted commands. */
  struct list_node free_command_list; /* Available commands. */
  struct rdr_imdraw_command buffer[]; /* Pool of allocated commands. */
};

/******************************************************************************
 *
 * Embedded shader.
 *
 ******************************************************************************/
static const char* imdraw3d_vs_source =
  "#version 330\n"
  "uniform mat4x4 transform;\n"
  "uniform vec4 color;\n"
  "layout(location = 0) in vec3 pos;\n"
  "flat out vec4 im_color;\n"
  "void main()\n"
  "{\n"
  " im_color = color;\n"
  " gl_Position = transform * vec4(pos, 1);\n"
  "}\n";

static const char* imdraw3d_fs_source =
  "#version 330\n"
  "flat in vec4 im_color;\n"
  "out vec4 color;\n"
  "void main()\n"
  "{\n"
  " color = im_color;\n"
  "}\n";

/******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
invoke_imdraw_parallelepiped
  (struct rdr_system* sys,
   struct rdr_imdraw_command* cmd)
{
  assert(sys && cmd && cmd->type == RDR_IMDRAW_PARALLELEPIPED);

  RBI(&sys->rb, bind_program(sys->ctxt, sys->im.draw3d.shading_program));
  RBI(&sys->rb, uniform_data
    (sys->im.draw3d.transform, 1, cmd->data.parallelepiped.transform));

  if(cmd->data.parallelepiped.solid_color[3] > 0.f) {
    RBI(&sys->rb, uniform_data
      (sys->im.draw3d.color, 1, cmd->data.parallelepiped.solid_color));

    if(cmd->data.parallelepiped.solid_color[3] >= 1.f) {
      RBU(draw_geometry(&sys->rbu.solid_parallelepiped));
    } else {
      struct rb_rasterizer_desc raster = {
        .fill_mode = RB_FILL_SOLID,
        .cull_mode = RB_CULL_FRONT,
        .front_facing = RB_ORIENTATION_CCW
      };
      struct rb_depth_stencil_desc depth_stencil = {
        .enable_depth_test = 1,
        .enable_depth_write = 0,
        .enable_stencil_test = 0,
        .depth_func = RB_COMPARISON_LESS_EQUAL
      };
      struct rb_blend_desc blend = {
        .enable = 1,
        .src_blend_RGB = RB_BLEND_SRC_ALPHA,
        .src_blend_Alpha = RB_BLEND_ZERO,
        .dst_blend_RGB = RB_BLEND_ONE_MINUS_SRC_ALPHA,
        .dst_blend_Alpha = RB_BLEND_ONE,
        .blend_op_RGB = RB_BLEND_OP_ADD,
        .blend_op_Alpha = RB_BLEND_OP_ADD
     };
      RBI(&sys->rb, depth_stencil(sys->ctxt, &depth_stencil));
      RBI(&sys->rb, blend(sys->ctxt, &blend));

      /* Draw back-facing triangles. */
      RBI(&sys->rb, rasterizer(sys->ctxt, &raster));
      RBU(draw_geometry(&sys->rbu.solid_parallelepiped));
      /* Draw front-facing triangles. */
      raster.cull_mode = RB_CULL_BACK;
      RBI(&sys->rb, rasterizer(sys->ctxt, &raster));
      RBU(draw_geometry(&sys->rbu.solid_parallelepiped));

      blend.enable = 0;
      RBI(&sys->rb, blend(sys->ctxt, &blend));
      depth_stencil.enable_depth_write = 1;
      RBI(&sys->rb, depth_stencil(sys->ctxt, &depth_stencil));
    }
  }
  if(cmd->data.parallelepiped.wire_color[3] > 0.f) {
    RBI(&sys->rb, uniform_data
      (sys->im.draw3d.color, 1, cmd->data.parallelepiped.wire_color));
    RBU(draw_geometry(&sys->rbu.wire_parallelepiped));
  }

  RBI(&sys->rb, bind_program(sys->ctxt, NULL));
}

static void
invoke_imdraw_circle
  (struct rdr_system* sys,
   struct rdr_imdraw_command* cmd)
{
  assert(sys && cmd && cmd->type == RDR_IMDRAW_CIRCLE);

  RBI(&sys->rb, bind_program(sys->ctxt, sys->im.draw3d.shading_program));
  RBI(&sys->rb, uniform_data
    (sys->im.draw3d.transform, 1, cmd->data.circle.transform));
  RBI(&sys->rb, uniform_data
    (sys->im.draw3d.color, 1, cmd->data.circle.color));
  RBU(draw_geometry(&sys->rbu.circle));
  RBI(&sys->rb, bind_program(sys->ctxt, NULL));
}

static void
release_imdraw_command_buffer(struct ref* ref)
{
  struct rdr_imdraw_command_buffer* cmdbuf = NULL;
  struct rdr_system* sys = NULL;
  assert(ref);

  cmdbuf = CONTAINER_OF(ref, struct rdr_imdraw_command_buffer, ref);
  sys = cmdbuf->sys;
  MEM_FREE(sys->allocator, cmdbuf);
  RDR(system_ref_put(sys));
}

/******************************************************************************
 *
 * Im Rendering.
 *
 ******************************************************************************/
enum rdr_error
rdr_init_im_rendering(struct rdr_system* sys)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(sys==NULL)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  RBI(&sys->rb, create_shader
    (sys->ctxt,
     RB_VERTEX_SHADER,
     imdraw3d_vs_source,
     strlen(imdraw3d_vs_source),
     &sys->im.draw3d.vertex_shader));
  RBI(&sys->rb, create_shader
    (sys->ctxt,
     RB_FRAGMENT_SHADER,
     imdraw3d_fs_source,
     strlen(imdraw3d_fs_source),
     &sys->im.draw3d.fragment_shader));
  RBI(&sys->rb, create_program(sys->ctxt, &sys->im.draw3d.shading_program));
  RBI(&sys->rb, attach_shader
    (sys->im.draw3d.shading_program, sys->im.draw3d.vertex_shader));
  RBI(&sys->rb, attach_shader
    (sys->im.draw3d.shading_program, sys->im.draw3d.fragment_shader));
  RBI(&sys->rb, link_program(sys->im.draw3d.shading_program));

  RBI(&sys->rb, get_named_uniform
    (sys->ctxt,
     sys->im.draw3d.shading_program,
     "transform",
     &sys->im.draw3d.transform));
  RBI(&sys->rb, get_named_uniform
    (sys->ctxt,
     sys->im.draw3d.shading_program,
     "color",
     &sys->im.draw3d.color));
exit:
  return rdr_err;
error:
  goto exit;
}

enum rdr_error
rdr_shutdown_im_rendering(struct rdr_system* sys)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  if(UNLIKELY(sys==NULL)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  if(sys->im.draw3d.vertex_shader)
    RBI(&sys->rb, shader_ref_put(sys->im.draw3d.vertex_shader));
  if(sys->im.draw3d.fragment_shader)
    RBI(&sys->rb, shader_ref_put(sys->im.draw3d.fragment_shader));
  if(sys->im.draw3d.shading_program)
    RBI(&sys->rb, program_ref_put(sys->im.draw3d.shading_program));
  if(sys->im.draw3d.transform)
    RBI(&sys->rb, uniform_ref_put(sys->im.draw3d.transform));
  if(sys->im.draw3d.color)
    RBI(&sys->rb, uniform_ref_put(sys->im.draw3d.color));
  memset(&sys->im.draw3d, 0, sizeof(sys->im.draw3d));
exit:
  return rdr_err;
error:
  goto exit;
}

/******************************************************************************
 *
 * Im draw command buffer functions.
 *
 ******************************************************************************/
enum rdr_error
rdr_create_imdraw_command_buffer
  (struct rdr_system* sys,
   size_t max_command_count,
   struct rdr_imdraw_command_buffer** out_cmdbuf)
{
  struct rdr_imdraw_command_buffer* cmdbuf = NULL;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(sys==NULL || max_command_count==0 || out_cmdbuf==NULL)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  cmdbuf = MEM_CALLOC
    (sys->allocator, 1,
     sizeof(struct rdr_imdraw_command_buffer) +
     max_command_count * sizeof(struct rdr_imdraw_command));
  if(!cmdbuf) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  ref_init(&cmdbuf->ref);
  list_init(&cmdbuf->emit_command_list);
  list_init(&cmdbuf->free_command_list);
  RDR(system_ref_get(sys));
  cmdbuf->sys = sys;
  cmdbuf->max_command_count = max_command_count;

  for(i = 0; i < max_command_count; ++i) {
    list_init(&cmdbuf->buffer[i].node);
    list_add_tail(&cmdbuf->free_command_list, &cmdbuf->buffer[i].node);
  }
exit:
  if(out_cmdbuf)
    *out_cmdbuf = cmdbuf;
  return rdr_err;
error:
  if(cmdbuf) {
    RDR(imdraw_command_buffer_ref_put(cmdbuf));
    cmdbuf = NULL;
  }
  goto exit;
}

enum rdr_error
rdr_imdraw_command_buffer_ref_get(struct rdr_imdraw_command_buffer* cmdbuf)
{
  if(UNLIKELY(cmdbuf==NULL))
    return RDR_INVALID_ARGUMENT;
  ref_get(&cmdbuf->ref);
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_imdraw_command_buffer_ref_put(struct rdr_imdraw_command_buffer* cmdbuf)
{
  if(UNLIKELY(cmdbuf==NULL))
    return RDR_INVALID_ARGUMENT;
  ref_put(&cmdbuf->ref, release_imdraw_command_buffer);
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_get_imdraw_command
  (struct rdr_imdraw_command_buffer* cmdbuf,
   struct rdr_imdraw_command** cmd)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(!cmdbuf|| !cmd)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  if(is_list_empty(&cmdbuf->free_command_list)) {
    *cmd = NULL;
  } else {
    struct list_node* node = list_head(&cmdbuf->free_command_list);
    list_del(node);
    *cmd = CONTAINER_OF(node, struct rdr_imdraw_command, node);
    (*cmd)->type = RDR_NB_IMDRAW_TYPES;
  }
exit:
  return rdr_err;
error:
  goto exit;
}

enum rdr_error
rdr_emit_imdraw_command
  (struct rdr_imdraw_command_buffer* cmdbuf,
   struct rdr_imdraw_command* cmd)
{
  if(UNLIKELY(cmdbuf==NULL || cmd==NULL || cmd->type==RDR_NB_IMDRAW_TYPES))
    return RDR_INVALID_ARGUMENT;

  /* Check that the command to emit is effectively get from cmdbuf. */
  if(UNLIKELY(false == IS_MEMORY_OVERLAPPED
    (cmd,
     sizeof(struct rdr_imdraw_command),
     cmdbuf->buffer,
     sizeof(struct rdr_imdraw_command) * cmdbuf->max_command_count))) {
    return RDR_INVALID_ARGUMENT;
  }
  list_add_tail(&cmdbuf->emit_command_list, &cmd->node);
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_flush_imdraw_command_buffer(struct rdr_imdraw_command_buffer* cmdbuf)
{
  struct list_node* node = NULL;
  struct list_node* tmp = NULL;

  if(UNLIKELY(cmdbuf == NULL))
    return RDR_INVALID_ARGUMENT;

  LIST_FOR_EACH_SAFE(node, tmp, &cmdbuf->emit_command_list) {
    struct rdr_imdraw_command* cmd = CONTAINER_OF
      (node, struct rdr_imdraw_command, node);
    switch(cmd->type) {
      case RDR_IMDRAW_CIRCLE:
        invoke_imdraw_circle(cmdbuf->sys, cmd);
        break;
      case RDR_IMDRAW_PARALLELEPIPED:
        invoke_imdraw_parallelepiped(cmdbuf->sys, cmd);
        break;
      default: assert(0); break;
    }
    list_move_tail(node, &cmdbuf->free_command_list);
  }
  return RDR_NO_ERROR;
}

