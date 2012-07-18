#include "renderer/regular/rdr_imdraw_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_imdraw.h"
#include "renderer/rdr_system.h"
#include "sys/mem_allocator.h"
#include <string.h>

struct rdr_imdraw_command_buffer {
  struct ref ref;
  struct rdr_system* sys;
  size_t max_command_count;
  struct list_node emit_command_list; /* Emitted cmds. */
  struct list_node emit_uppermost_command_list; /* Emitted front layer cmds. */
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
  " gl_Position = transform * vec4(pos, 1.f);\n"
  "}\n";

static const char* imdraw2d_vs_source =
  "#version 330\n"
  "uniform mat4x4 transform;\n"
  "uniform vec4 color;\n"
  "layout(location = 0) in vec2 pos;\n"
  "flat out vec4 im_color;\n"
  "void main()\n"
  "{\n"
  " im_color = color;\n"
  " gl_Position = transform * vec4(pos, vec2(0.f, 1.f));\n"
  "}\n";

static const char* imdraw2d_color_vs_source =
  "#version 330\n"
  "uniform mat4x4 transform;\n"
  "layout(location = 0) in vec2 pos;\n"
  "layout(location = 1) in vec3 col;\n"
  "flat out vec4 im_color;\n"
  "void main()\n"
  "{\n"
  " im_color = vec4(col, 1.f);\n"
  " gl_Position = transform * vec4(pos, vec2(0.f, 1.f));\n"
  "}\n";

static const char* imdraw_fs_source =
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

}

static void
invoke_imdraw_circle
  (struct rdr_system* sys,
   struct rdr_imdraw_command* cmd)
{
  assert(sys && cmd && cmd->type == RDR_IMDRAW_CIRCLE);

  RBI(&sys->rb, bind_program(sys->ctxt, sys->im.draw2d.shading_program));
  RBI(&sys->rb, uniform_data
    (sys->im.draw2d.transform, 1, cmd->data.circle.transform));
  RBI(&sys->rb, uniform_data
    (sys->im.draw2d.color, 1, cmd->data.circle.color));
  RBU(draw_geometry(&sys->rbu.circle));
}

static void
setup_im_grid
  (struct rdr_system* sys,
   const float* vertices, /* Interleved list of 2D positions and RGB colors */
   unsigned int nvertices)
{
  assert(sys && (vertices || !nvertices));

  if(nvertices == 0) {
    sys->im.grid.nvertices = 0;
  } else {
    const size_t sizeof_vertex = 5 /* 2D pos + RGB color */ * sizeof(float);
    const size_t sizeof_vertices = nvertices * sizeof_vertex;
    const struct rb_buffer_attrib buf_attr[2] = {
       [0] = {
         .index = 0,
         .stride = sizeof_vertex,
         .offset = 0,
         .type = RB_FLOAT2
       },
       [1] = {
         .index = 1,
         .stride = sizeof_vertex,
         .offset = 2 * sizeof(float),
         .type = RB_FLOAT3
       }
     };
    /* Setup vertex buffer */
    if(sizeof_vertices <= sys->im.grid.sizeof_vertex_buffer) {
      RBI(&sys->rb, buffer_data
        (sys->im.grid.vertex_buffer, 0, sizeof_vertices, vertices));
    } else {
      const struct rb_buffer_desc buf_desc = {
        .size = sizeof_vertices,
        .target = RB_BIND_VERTEX_BUFFER,
        .usage = RB_USAGE_IMMUTABLE
      };
      if(sys->im.grid.vertex_buffer != NULL) {
        RBI(&sys->rb, buffer_ref_put(sys->im.grid.vertex_buffer));
      }
      RBI(&sys->rb, create_buffer
        (sys->ctxt, &buf_desc, vertices, &sys->im.grid.vertex_buffer));
      sys->im.grid.sizeof_vertex_buffer = sizeof_vertices;
    }
    sys->im.grid.nvertices = nvertices;
    /* Setup vertex array */
    if(sys->im.grid.vertex_array == NULL) {
      RBI(&sys->rb, create_vertex_array(sys->ctxt, &sys->im.grid.vertex_array));
    }
    RBI(&sys->rb, vertex_attrib_array
      (sys->im.grid.vertex_array, sys->im.grid.vertex_buffer, 2, buf_attr));
  }
}

static void
build_im_grid(struct rdr_system* sys, const struct rdr_imdraw_command* cmd)
{
  float* vertices = NULL;
  unsigned int nvertices = 0;
  const struct rdr_im_grid_desc* grid_desc = NULL;
  assert(cmd && sys);

  grid_desc = &cmd->data.grid.desc;

  if(0 != memcmp
     (&sys->im.grid.desc,
      &cmd->data.grid.desc,
      sizeof(struct rdr_im_grid_desc))) {
    if(cmd->data.grid.desc.ndiv[0] && cmd->data.grid.desc.ndiv[1]) {
      float* vertex = NULL;
      size_t line_id = 0, subline_id = 0;

      /* Precompute several values */
      const float step[2] = {
        1.f / (float)grid_desc->ndiv[0],
        1.f / (float)grid_desc->ndiv[1]
      };
      const float substep[2] = {
        step[0] / (float)grid_desc->nsubdiv[0],
        step[1] / (float)grid_desc->nsubdiv[1]
      };
      const unsigned int nfloat_per_vertex = 5; /* 2D position + RGB color */
      const unsigned int nb_vlines = grid_desc->ndiv[0] + 1;
      const unsigned int nb_hlines = grid_desc->ndiv[1] + 1;
      const unsigned int nb_vsublines =
        grid_desc->nsubdiv[0] - (grid_desc->nsubdiv[0] > 0);
      const unsigned int nb_hsublines =
        grid_desc->nsubdiv[1] - (grid_desc->nsubdiv[1] > 0);
      const unsigned int total_nb_lines =
          nb_vlines
        + nb_hlines
        + (grid_desc->ndiv[0] * nb_vsublines)
        + (grid_desc->ndiv[1] * nb_hsublines)
        + 2 /* axis */;

      /* Allocate temporary client side vertex buffer */
      nvertices = total_nb_lines * 2;
      vertices = MEM_ALLOC
        (sys->allocator, nvertices * nfloat_per_vertex * sizeof(float));
      assert(vertices != NULL);
      vertex = vertices;

      #define FARRAY(...) ((float[]){__VA_ARGS__})
      #define PUSH_VERTEX(vertex, xy, rgb) \
      do { \
        (vertex)[0] = (xy)[0]; \
        (vertex)[1] = (xy)[1]; \
        (vertex)[2] = (rgb)[0]; \
        (vertex)[3] = (rgb)[1]; \
        (vertex)[4] = (rgb)[2]; \
        (vertex) += nfloat_per_vertex; \
      } while(0)

      /* Setup vertical lines */
      for(line_id = 0;  line_id < nb_vlines; ++line_id) {
        float x = -0.5f + line_id * step[0];
        PUSH_VERTEX(vertex, FARRAY(x, 0.5f), grid_desc->div_color);
        PUSH_VERTEX(vertex, FARRAY(x,-0.5f), grid_desc->div_color);
        if(line_id < nb_vlines - 1) {
          x += substep[0];
          /* Setup sub vertical lines */
          for(subline_id = 0; subline_id < nb_vsublines; ++subline_id) {
            const float subx = x + subline_id * substep[0];
            PUSH_VERTEX(vertex, FARRAY(subx, 0.5f), grid_desc->subdiv_color);
            PUSH_VERTEX(vertex, FARRAY(subx,-0.5f), grid_desc->subdiv_color);
          }
        }
      }
      /* Setup horizontal lines */
     for(line_id = 0; line_id < nb_hlines; ++line_id) {
        float y = -0.5f + line_id * step[1];
        PUSH_VERTEX(vertex, FARRAY( 0.5f,y), grid_desc->div_color);
        PUSH_VERTEX(vertex, FARRAY(-0.5f,y), grid_desc->div_color);
        if(line_id < nb_hlines - 1) {
          y += substep[1];
          for(subline_id = 0; subline_id < nb_hsublines; ++subline_id) {
            const float suby = y + subline_id * substep[1];
            PUSH_VERTEX(vertex, FARRAY( 0.5f,suby), grid_desc->subdiv_color);
            PUSH_VERTEX(vertex, FARRAY(-0.5f,suby), grid_desc->subdiv_color);
          }
        }
      }
      /* Vertical axis */
      PUSH_VERTEX(vertex, FARRAY(0.f, 0.5f), grid_desc->vaxis_color);
      PUSH_VERTEX(vertex, FARRAY(0.f,-0.5f), grid_desc->vaxis_color);
      /* Horizontal axis */
      PUSH_VERTEX(vertex, FARRAY( 0.5f,0.f), grid_desc->haxis_color);
      PUSH_VERTEX(vertex, FARRAY(-0.5f,0.f), grid_desc->haxis_color);
    }

    #undef FARRAY
    #undef PUSH_VERTEX
    setup_im_grid(sys, vertices, nvertices);
    memcpy(&sys->im.grid.desc, grid_desc, sizeof(struct rdr_im_grid_desc));
    if(vertices)
      MEM_FREE(sys->allocator, vertices);
  }
}

static void
invoke_imdraw_grid
  (struct rdr_system* sys,
   struct rdr_imdraw_command* cmd)
{
  assert(sys && cmd && cmd->type == RDR_IMDRAW_GRID);

  build_im_grid(sys, cmd);
  if(sys->im.grid.nvertices != 0) {
    RBI(&sys->rb, bind_program
      (sys->ctxt, sys->im.draw2d_color.shading_program));
    RBI(&sys->rb, uniform_data
      (sys->im.draw2d_color.transform, 1, cmd->data.grid.transform));
    RBI(&sys->rb, bind_vertex_array(sys->ctxt, sys->im.grid.vertex_array));
    RBI(&sys->rb, draw(sys->ctxt, RB_LINES, sys->im.grid.nvertices));
  }
}

static void
flush_command_list
  (struct rdr_imdraw_command_buffer* cmdbuf,
   struct list_node* cmdlist)
{
  struct rb_viewport_desc viewport_desc;
  struct list_node* node = NULL;
  struct list_node* tmp = NULL;
  memset(&viewport_desc, 0, sizeof(viewport_desc));
  assert(cmdbuf && cmdlist);

  viewport_desc.min_depth = 0.f;
  viewport_desc.max_depth = 1.f;

  LIST_FOR_EACH_SAFE(node, tmp, cmdlist) {
    struct rdr_imdraw_command* cmd = CONTAINER_OF
      (node, struct rdr_imdraw_command, node);

    if(((int)cmd->viewport[0] != viewport_desc.x)
     | ((int)cmd->viewport[1] != viewport_desc.y)
     | ((int)cmd->viewport[2] != viewport_desc.width)
     | ((int)cmd->viewport[3] != viewport_desc.height)) {
      viewport_desc.x = cmd->viewport[0];
      viewport_desc.y = cmd->viewport[1];
      viewport_desc.width = cmd->viewport[2];
      viewport_desc.height = cmd->viewport[3];
      RBI(&cmdbuf->sys->rb, viewport(cmdbuf->sys->ctxt, &viewport_desc));
    }
    switch(cmd->type) {
      case RDR_IMDRAW_CIRCLE:
        invoke_imdraw_circle(cmdbuf->sys, cmd);
        break;
      case RDR_IMDRAW_GRID:
        invoke_imdraw_grid(cmdbuf->sys, cmd);
        break;
      case RDR_IMDRAW_PARALLELEPIPED:
        invoke_imdraw_parallelepiped(cmdbuf->sys, cmd);
        break;
      default: assert(0); break;
    }
    list_move_tail(node, &cmdbuf->free_command_list);
  }
  RBI(&cmdbuf->sys->rb, bind_program(cmdbuf->sys->ctxt, NULL));
}

enum im_draw_flag {
  IM_DRAW_NONE = 0,
  IM_DRAW_HAS_COLOR_UNIFORM = BIT(0)
};

static void
init_im_draw
  (struct rbi* rbi,
   struct rb_context* ctxt,
   struct im_draw* im_draw,
   const char* vs_source,
   const char* fs_source,
   int flag)
{
  assert(rbi && im_draw && vs_source && fs_source);

  RBI(rbi, create_shader
    (ctxt, RB_VERTEX_SHADER,
     vs_source,
     strlen(vs_source),
     &im_draw->vertex_shader));
  RBI(rbi, create_shader
    (ctxt, RB_FRAGMENT_SHADER,
     fs_source,
     strlen(fs_source),
     &im_draw->fragment_shader));
  RBI(rbi, create_program(ctxt, &im_draw->shading_program));
  RBI(rbi, attach_shader(im_draw->shading_program, im_draw->vertex_shader));
  RBI(rbi, attach_shader(im_draw->shading_program, im_draw->fragment_shader));
  RBI(rbi, link_program(im_draw->shading_program));
  RBI(rbi, get_named_uniform
    (ctxt, im_draw->shading_program, "transform", &im_draw->transform));

  if(flag & IM_DRAW_HAS_COLOR_UNIFORM) {
    RBI(rbi, get_named_uniform
      (ctxt, im_draw->shading_program, "color", &im_draw->color));
  }
}

static void
release_im_draw(struct rbi* rbi, struct im_draw* im_draw)
{
  assert(rbi && im_draw);

  if(im_draw->vertex_shader)
    RBI(rbi, shader_ref_put(im_draw->vertex_shader));
  if(im_draw->fragment_shader)
    RBI(rbi, shader_ref_put(im_draw->fragment_shader));
  if(im_draw->shading_program)
    RBI(rbi, program_ref_put(im_draw->shading_program));
  if(im_draw->transform)
    RBI(rbi, uniform_ref_put(im_draw->transform));
  if(im_draw->color)
    RBI(rbi, uniform_ref_put(im_draw->color));
  memset(im_draw, 0, sizeof(struct im_draw));
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
  if(UNLIKELY(sys==NULL))
    return RDR_INVALID_ARGUMENT;

  init_im_draw
    (&sys->rb,
     sys->ctxt,
     &sys->im.draw2d,
     imdraw2d_vs_source,
     imdraw_fs_source,
     IM_DRAW_HAS_COLOR_UNIFORM);
  init_im_draw
    (&sys->rb,
     sys->ctxt,
     &sys->im.draw3d,
     imdraw3d_vs_source,
     imdraw_fs_source,
     IM_DRAW_HAS_COLOR_UNIFORM);
  init_im_draw
    (&sys->rb,
     sys->ctxt,
     &sys->im.draw2d_color,
     imdraw2d_color_vs_source,
     imdraw_fs_source,
     IM_DRAW_NONE);

  return RDR_NO_ERROR;
}

enum rdr_error
rdr_shutdown_im_rendering(struct rdr_system* sys)
{
  if(UNLIKELY(sys==NULL))
    return RDR_INVALID_ARGUMENT;
  release_im_draw(&sys->rb, &sys->im.draw2d);
  release_im_draw(&sys->rb, &sys->im.draw3d);
  release_im_draw(&sys->rb, &sys->im.draw2d_color);
  if(sys->im.grid.vertex_buffer)
    RBI(&sys->rb, buffer_ref_put(sys->im.grid.vertex_buffer));
  if(sys->im.grid.vertex_array)
    RBI(&sys->rb, vertex_array_ref_put(sys->im.grid.vertex_array));
  return RDR_NO_ERROR;
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
  list_init(&cmdbuf->emit_uppermost_command_list);
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
    (*cmd)->flag = RDR_IMDRAW_FLAG_NONE;
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
  if((cmd->flag & RDR_IMDRAW_FLAG_UPPERMOST_LAYER) != 0) {
    list_add_tail(&cmdbuf->emit_uppermost_command_list, &cmd->node);
  } else {
    list_add_tail(&cmdbuf->emit_command_list, &cmd->node);
  }
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_flush_imdraw_command_buffer(struct rdr_imdraw_command_buffer* cmdbuf)
{
  if(UNLIKELY(cmdbuf == NULL))
    return RDR_INVALID_ARGUMENT;

  flush_command_list(cmdbuf, &cmdbuf->emit_command_list);
  if(!is_list_empty(&cmdbuf->emit_uppermost_command_list)) {
    const struct rb_depth_stencil_desc depth_stencil_desc = {
      .enable_depth_test = 1,
      .enable_depth_write = 1,
      .enable_stencil_test = 0,
      .front_face_op.write_mask = 0,
      .back_face_op.write_mask = 0,
      .depth_func = RB_COMPARISON_LESS_EQUAL
    };
    RBI(&cmdbuf->sys->rb, depth_stencil
      (cmdbuf->sys->ctxt, &depth_stencil_desc));
    RBI(&cmdbuf->sys->rb, clear
      (cmdbuf->sys->ctxt, RB_CLEAR_DEPTH_BIT, NULL, 1.f, 0x00));
    flush_command_list(cmdbuf, &cmdbuf->emit_uppermost_command_list);
  }
  return RDR_NO_ERROR;
}

