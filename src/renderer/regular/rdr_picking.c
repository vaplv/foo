#include "renderer/regular/rdr_attrib_c.h"
#include "renderer/regular/rdr_draw_desc.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_imdraw_c.h"
#include "renderer/regular/rdr_picking.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/regular/rdr_uniform.h"
#include "renderer/regular/rdr_world_c.h"
#include "renderer/rdr_model_instance.h"
#include "render_backend/rbi.h"
#include "renderer/rdr.h"
#include "renderer/rdr_world.h"
#include "stdlib/sl.h"
#include "stdlib/sl_vector.h"
#include "sys/math.h"
#include <assert.h>
#include <string.h>

enum { PICK_UNIFORM_MVP, PICK_UNIFORM_MDL_ID, NB_PICK_UNIFORMS };

#define NB_RESULT_BUFFERS 2 /* Use double buffering */

struct rdr_picking {
  struct rdr_picking_desc desc;
  struct result {
    struct sl_vector* buffer_list[NB_RESULT_BUFFERS]; /* vectors of uint32 */
    uint8_t buffer_id;
  } result;
  struct framebuffer {
    struct rb_framebuffer* buffer;
    struct rb_tex2d* picking_tex;
    struct rb_tex2d* depth_stencil_tex;
  } framebuffer;
  struct shading {
    struct rb_program* program;
    struct rb_shader* vertex_shader;
    struct rb_shader* fragment_shader;
    struct rdr_uniform uniform_list[NB_PICK_UNIFORMS];
  } shading;
  struct debug {
    struct rb_program* program;
    struct rb_shader* vertex_shader;
    struct rb_shader* fragment_shader;
    struct rb_uniform* scale_bias_uniform;
    struct rb_uniform* max_pick_id_uniform;
    struct rb_uniform* pick_buffer_uniform;
    struct rb_uniform* depth_buffer_uniform;
    struct rb_sampler* sampler;
  } debug;
};

/*******************************************************************************
 *
 * Embedded shaders.
 *
 ******************************************************************************/
static const char* picking_vs_source =
  "#version 330\n"
  "layout(location =" STR(RDR_ATTRIB_POSITION_ID) ") in vec3 pos;\n"
  "uniform uint in_id;\n"
  "uniform mat4x4 mvp;\n"
  "flat out uint id;\n"
  "void main()\n"
  "{\n"
  " id = in_id;\n"
  " gl_Position = mvp * vec4(pos, 1.f);\n"
  "}\n";

static const char* picking_fs_source =
  "#version 330\n"
  "flat in uint id;\n"
  "out uint out_id;\n"
  "void main()\n"
  "{\n"
  " out_id = id;\n"
  "}\n";

static const char* show_picking_vs_source =
  "#version 330\n"
  "layout(location = 0) in vec2 pos;\n"
  "void main()\n"
  "{\n"
  " gl_Position = vec4(pos * vec2(2.f) - vec2(1.f), vec2(0.f, 1.f));\n"
  "}\n";

static const char* show_picking_fs_source =
  "#version 330\n"
  "uniform usampler2D pick_buffer;\n"
  "uniform sampler2D depth_buffer;\n"
  "out vec4 color;\n"
  "uniform vec4 scale_bias;\n"
  "uniform uint max_pick_id;\n"
  "void main()\n"
  "{\n"
  "  vec2 uv = gl_FragCoord.xy * scale_bias.xy + scale_bias.zw;\n"
  "  uint val = texture(pick_buffer, uv).r;\n"
  "  float normed_val = val / float(max_pick_id);\n"
  "  float depth = texture(depth_buffer, uv).x;\n"
  "\n"
  "  if(depth == 1.f) {\n"
  "    color.rgb = vec3(0.f);\n"
  "  } else if(normed_val < 1.f / 3.f) { \n"
  "    float u = normed_val * 3.f;\n"
  "    color.rgb = u*vec3(0.f, 0.f, 1.f) + (1.f - u)*vec3(0.f, 0.f, 0.f);\n"
  "  } else if(normed_val < 2.f / 3.f) {\n"
  "    float u = (normed_val - 1.f/3.f) * 3.f;\n"
  "    color.rgb = u*vec3(0.f, 1.f, 0.f) + (1.f - u)*vec3(0.f, 0.f, 1.f);\n"
  "  } else { \n"
  "    float u = (normed_val - 2.f/3.f) * 3.f;\n"
  "    color.rgb = u*vec3(1.f, 0.f, 0.f) + (1.f - u)*vec3(0.f, 1.f, 0.f);\n"
  "  }\n"
  "  color.a = 1.f;\n"
  "  gl_FragDepth = texture(depth_buffer, uv).x;\n"
  "}\n";

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static enum rdr_error
draw_picked_world
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   struct rdr_world* world,
   const struct rdr_view* view)
{
  const struct rb_clear_framebuffer_color_desc clear_desc = {
    .index = 0,
    .val = { .rgba_ui32 = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX } }
  };
  struct rdr_draw_desc draw_desc;
  struct rdr_view pick_view;
  memset(&draw_desc, 0, sizeof(struct rdr_draw_desc));

  assert(sys && picking && world && view);

  /* Adjust the input viewport to match the pick definition. */
  memcpy(&pick_view, view, sizeof(struct rdr_view));
  pick_view.x = pick_view.y = 0;
  pick_view.width = picking->desc.width;
  pick_view.height = picking->desc.height;

  /* Define the draw desc. */
  draw_desc.uniform_list = picking->shading.uniform_list;
  draw_desc.nb_uniforms = NB_PICK_UNIFORMS;
  draw_desc.bind_flag = RDR_BIND_ATTRIB_POSITION;

  /* Draw the world into the picking buffer. */
  RBI(&sys->rb, clear_framebuffer_render_targets
    (picking->framebuffer.buffer,
     RB_CLEAR_COLOR_BIT | RB_CLEAR_DEPTH_BIT,
     1, &clear_desc, 1.f, 0));

  RBI(&sys->rb, bind_framebuffer(sys->ctxt, picking->framebuffer.buffer));
  RBI(&sys->rb, bind_program(sys->ctxt, picking->shading.program));
  RDR(draw_world(world, &pick_view, &draw_desc));
  RBI(&sys->rb, bind_framebuffer(sys->ctxt, NULL));
  RBI(&sys->rb, bind_program(sys->ctxt, NULL));

  return RDR_NO_ERROR;
}

static enum rdr_error
draw_picked_imdraw
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   struct rdr_imdraw_command_buffer* cmdbuf)
{
  const struct rb_clear_framebuffer_color_desc clear_desc = {
    .index = 0,
    .val = { .rgba_ui32 = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX } }
  };

  assert(sys && picking);

  /* Draw the world into the picking buffer. */
  RBI(&sys->rb, clear_framebuffer_render_targets
    (picking->framebuffer.buffer,
     RB_CLEAR_COLOR_BIT | RB_CLEAR_DEPTH_BIT,
     1, &clear_desc, 1.f, 0));

  RBI(&sys->rb, bind_framebuffer(sys->ctxt, picking->framebuffer.buffer));
  RBI(&sys->rb, bind_program(sys->ctxt, picking->shading.program));
  RDR(execute_imdraw_command_buffer(cmdbuf, RDR_IMDRAW_EXEC_FLAG_PICKING));
  RBI(&sys->rb, bind_framebuffer(sys->ctxt, NULL));
  RBI(&sys->rb, bind_program(sys->ctxt, NULL));

  return RDR_NO_ERROR;

}

static FINLINE void
result_swap_buffer(struct result* result)
{
  assert(result);
  result->buffer_id  = (result->buffer_id + 1) % NB_RESULT_BUFFERS;
  SL(clear_vector(result->buffer_list[result->buffer_id]));
}

static FINLINE struct sl_vector*
result_get_buffer(struct result* result)
{
  assert(result);
  return result->buffer_list[result->buffer_id];
}

static enum rdr_error
read_back_picking_buffer
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   const struct rdr_view* view, /* May be NULL */
   const unsigned int pos[2],
   const unsigned int size[2])
{
  struct sl_vector* pick_id_list = NULL;
  uint32_t* buf = NULL;
  size_t bufsize = 0;
  float scale[2] = {0.f, 0.f}; /* scale factor from viewport to pick space. */
  unsigned int pick_pos[2] = {0, 0}; /* position in viewport space. */
  unsigned int blit_pos0[2] = {0, 0};
  unsigned int blit_pos1[2] = {0, 0};
  unsigned int blit_size[2] = {0, 0};

  assert(sys && picking && pos && size);

  if(view == NULL) {
    scale[0] = 1.f;
    scale[1] = 1.f;
    pick_pos[0] = pos[0];
    pick_pos[1] = pos[1];
  } else {
    /* Compute the viewport to pick spcae scale factor. */
    scale[0] = (float)picking->desc.width / (float)(view->width - view->x);
    scale[1] = (float)picking->desc.height / (float)(view->height - view->y);
    /* Compute the pick position in pick space. */
    pick_pos[0] = pos[0] - view->x;
    pick_pos[1] = pos[1] - view->y;
  }

  /* Defint the blit rectangle in pick space. */
  blit_pos0[0] = pick_pos[0] * scale[0];
  blit_pos0[1] = pick_pos[1] * scale[1];
  blit_pos1[0] = blit_pos0[0] + MAX(size[0] * scale[0], 1);
  blit_pos1[1] = blit_pos0[1] + MAX(size[1] * scale[1], 1);
  blit_pos1[0] = MIN(blit_pos1[0], picking->desc.width);
  blit_pos1[1] = MIN(blit_pos1[1], picking->desc.height);
  blit_size[0] = blit_pos1[0] - blit_pos0[0];
  blit_size[1] = blit_pos1[1] - blit_pos0[1];

  /* Read back pick content. */
  pick_id_list = result_get_buffer(&picking->result);
  SL(clear_vector(pick_id_list));
  SL(vector_push_back_n
    (pick_id_list,
     blit_size[0] * blit_size[1],
     (uint32_t[]){UINT32_MAX}));
  SL(vector_buffer(pick_id_list, &bufsize, NULL, NULL, (void**)&buf));
#ifndef NDEBUG
  {
    size_t tmp = 0;
    RBI(&sys->rb, read_back_framebuffer
      (picking->framebuffer.buffer,
       0, blit_pos0[0], blit_pos0[1], blit_size[0], blit_size[1], &tmp, NULL));
    assert(tmp <= bufsize * sizeof(uint32_t));
  }
#endif
  RBI(&sys->rb, read_back_framebuffer
    (picking->framebuffer.buffer,
     0, blit_pos0[0], blit_pos0[1], blit_size[0], blit_size[1], NULL, buf));

  return RDR_NO_ERROR;
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

  memset(framebuffer, 0, sizeof(struct framebuffer));
}

static enum rdr_error
init_framebuffer
  (struct rdr_system* sys,
   unsigned int width,
   unsigned int height,
   struct framebuffer* framebuffer)
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

  assert(sys && width && height && framebuffer);

  #define CALL(func) \
    do { \
      if((func) != 0) { \
        rdr_err = RDR_DRIVER_ERROR; \
        goto error; \
      } \
    } while(0)

  bufdesc.width = width;
  bufdesc.height = height;
  bufdesc.sample_count = 1;
  bufdesc.buffer_count = 1;
  CALL(sys->rb.create_framebuffer(sys->ctxt, &bufdesc, &framebuffer->buffer));

  tex2ddesc.width = width;
  tex2ddesc.height = height;
  tex2ddesc.mip_count = 1;
  tex2ddesc.format = RB_R_UINT32;
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
  goto exit;
}

static void
release_shading(struct rdr_system* sys, struct shading* shading)
{
  size_t i = 0;
  assert(sys && shading);

  if(shading->vertex_shader)
    RBI(&sys->rb, shader_ref_put(shading->vertex_shader));
  if(shading->fragment_shader)
    RBI(&sys->rb, shader_ref_put(shading->fragment_shader));
  if(shading->program)
    RBI(&sys->rb, program_ref_put(shading->program));

  for(i = 0; i < NB_PICK_UNIFORMS; ++i) {
    if(shading->uniform_list[i].uniform)
      RBI(&sys->rb, uniform_ref_put(shading->uniform_list[i].uniform));
  }
  memset(shading, 0, sizeof(struct shading));
}

static enum rdr_error
init_shading(struct rdr_system* sys, struct shading* shading)
{
  assert(sys && shading);

  /* Compile the shader sources. */
  RBI(&sys->rb, create_shader
    (sys->ctxt,
     RB_VERTEX_SHADER,
     picking_vs_source,
     strlen(picking_vs_source),
     &shading->vertex_shader));
  RBI(&sys->rb, create_shader
    (sys->ctxt,
     RB_FRAGMENT_SHADER,
     picking_fs_source,
     strlen(picking_fs_source),
     &shading->fragment_shader));

  /* Link a shading program from the compiled shader source. */
  RBI(&sys->rb, create_program(sys->ctxt, &shading->program));
  RBI(&sys->rb, attach_shader(shading->program, shading->vertex_shader));
  RBI(&sys->rb, attach_shader(shading->program, shading->fragment_shader));
  RBI(&sys->rb, link_program(shading->program));

  /* Retrieve the uniform of the shading program. */
  shading->uniform_list[PICK_UNIFORM_MDL_ID].usage = RDR_PICK_ID_UNIFORM;
  RBI(&sys->rb, get_named_uniform
    (sys->ctxt, shading->program, "in_id",
     &shading->uniform_list[PICK_UNIFORM_MDL_ID].uniform));
  shading->uniform_list[PICK_UNIFORM_MVP].usage = RDR_MODELVIEWPROJ_UNIFORM;
  RBI(&sys->rb, get_named_uniform
    (sys->ctxt, shading->program, "mvp",
     &shading->uniform_list[PICK_UNIFORM_MVP].uniform));

  return RDR_NO_ERROR;
}

static void
release_debug(struct rdr_system* sys, struct debug* debug)
{
  assert(sys && debug);

  if(debug->vertex_shader)
    RBI(&sys->rb, shader_ref_put(debug->vertex_shader));
  if(debug->fragment_shader)
    RBI(&sys->rb, shader_ref_put(debug->fragment_shader));
  if(debug->program)
    RBI(&sys->rb, program_ref_put(debug->program));
  if(debug->scale_bias_uniform)
    RBI(&sys->rb, uniform_ref_put(debug->scale_bias_uniform));
  if(debug->max_pick_id_uniform)
    RBI(&sys->rb, uniform_ref_put(debug->max_pick_id_uniform));
  if(debug->pick_buffer_uniform)
    RBI(&sys->rb, uniform_ref_put(debug->pick_buffer_uniform));
  if(debug->depth_buffer_uniform)
    RBI(&sys->rb, uniform_ref_put(debug->depth_buffer_uniform));
  if(debug->sampler)
    RBI(&sys->rb, sampler_ref_put(debug->sampler));
  memset(debug, 0, sizeof(struct debug));
}

static enum rdr_error
init_debug(struct rdr_system* sys, struct debug* debug)
{
  struct rb_sampler_desc sampler_desc;
  memset(&sampler_desc, 0, sizeof(struct rb_sampler_desc));
  assert(sys && debug);

  /* Compile the shader sources. */
  RBI(&sys->rb, create_shader
    (sys->ctxt,
     RB_VERTEX_SHADER,
     show_picking_vs_source,
     strlen(show_picking_vs_source),
     &debug->vertex_shader));
  RBI(&sys->rb, create_shader
    (sys->ctxt,
     RB_FRAGMENT_SHADER,
     show_picking_fs_source,
     strlen(show_picking_fs_source),
     &debug->fragment_shader));

  /* Link a shading program from the compiled shader source. */
  RBI(&sys->rb, create_program(sys->ctxt, &debug->program));
  RBI(&sys->rb, attach_shader(debug->program, debug->vertex_shader));
  RBI(&sys->rb, attach_shader(debug->program, debug->fragment_shader));
  RBI(&sys->rb, link_program(debug->program));
  RBI(&sys->rb, get_named_uniform
    (sys->ctxt, debug->program, "scale_bias", &debug->scale_bias_uniform));
  RBI(&sys->rb, get_named_uniform
    (sys->ctxt, debug->program, "max_pick_id", &debug->max_pick_id_uniform));
  RBI(&sys->rb, get_named_uniform
    (sys->ctxt, debug->program, "pick_buffer", &debug->pick_buffer_uniform));
  RBI(&sys->rb, get_named_uniform
    (sys->ctxt, debug->program, "depth_buffer", &debug->depth_buffer_uniform));

  /* Create the sampler. */
  sampler_desc.filter = RB_MIN_POINT_MAG_POINT_MIP_POINT;
  sampler_desc.address_u = RB_ADDRESS_CLAMP;
  sampler_desc.address_v = RB_ADDRESS_CLAMP;
  sampler_desc.address_w = RB_ADDRESS_CLAMP;
  sampler_desc.lod_bias = 0;
  sampler_desc.min_lod = 0.f;
  sampler_desc.max_lod = 1.f;
  sampler_desc.max_anisotropy = 1;
  RBI(&sys->rb, create_sampler(sys->ctxt, &sampler_desc, &debug->sampler));

  return RDR_NO_ERROR;
}

static void
release_result(struct rdr_system* sys UNUSED, struct result* result)
{
  uint8_t i = 0;
  STATIC_ASSERT( NB_RESULT_BUFFERS < 255, Unexpected_value );

  assert(sys && result);
  for(i = 0; i < NB_RESULT_BUFFERS; ++i) {
    if(result->buffer_list[i]) {
      SL(free_vector(result->buffer_list[i]));
    }
  }
  memset(result, 0, sizeof(struct result));
}

static enum rdr_error
init_result(struct rdr_system* sys, struct result* result)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  uint8_t i = 0;
  assert(sys && result);
  STATIC_ASSERT( NB_RESULT_BUFFERS < 255, Unexpected_value );

  for(i = 0; i < NB_RESULT_BUFFERS; ++i) {
    sl_err = sl_create_vector
      (sizeof(uint32_t),
       16,
       sys->allocator,
       &result->buffer_list[i]);
    if(sl_err != SL_NO_ERROR) {
      rdr_err = sl_to_rdr_error(sl_err);
      goto error;
    }
  }
exit:
  return rdr_err;
error:
  release_result(sys, result);
  goto exit;
}

/*******************************************************************************
 *
 * Picking functions
 *
 ******************************************************************************/
enum rdr_error
rdr_create_picking
  (struct rdr_system* sys,
   const struct rdr_picking_desc* desc,
   struct rdr_picking** out_picking)
{
  struct rdr_picking* picking = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(!sys || !out_picking)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  picking = MEM_CALLOC(sys->allocator, 1, sizeof(struct rdr_picking));
  if(!picking) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  picking->desc = *desc;
  rdr_err = init_framebuffer
    (sys, desc->width, desc->height, &picking->framebuffer);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = init_shading(sys, &picking->shading);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = init_debug(sys, &picking->debug);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = init_result(sys, &picking->result);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  if(out_picking)
    *out_picking = picking;
  return rdr_err;
error:
  if(picking) {
    RDR(free_picking(sys, picking));
    picking =  NULL;
  }
  goto exit;
}

enum rdr_error
rdr_free_picking(struct rdr_system* sys, struct rdr_picking* picking)
{
  if(!sys || !picking)
    return RDR_INVALID_ARGUMENT;
  release_framebuffer(sys, &picking->framebuffer);
  release_shading(sys, &picking->shading);
  release_debug(sys, &picking->debug);
  release_result(sys, &picking->result);
  MEM_FREE(sys->allocator, picking);
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_pick
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   struct rdr_world* world,
   const struct rdr_view* view,
   const unsigned int pos[2],
   const unsigned int size[2])
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(!sys || !picking || !world || !view || !pos || !size)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  if(pos[0] < view->x || pos[1] < view->y || size[0] == 0 || size[1] == 0)
    goto exit;

  rdr_err = draw_picked_world(sys, picking, world, view);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = read_back_picking_buffer(sys, picking, view, pos, size);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  return rdr_err;
error:
  if(sys && picking) {
    SL(clear_vector(result_get_buffer(&picking->result)));
  }
  goto exit;
}

enum rdr_error
rdr_pick_imdraw
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   struct rdr_imdraw_command_buffer* cmdbuf,
   const unsigned int pos[2],
   const unsigned int size[2])
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(!sys || !picking || !pos || !size)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  rdr_err = draw_picked_imdraw(sys, picking, cmdbuf);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = read_back_picking_buffer(sys, picking, NULL, pos, size);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  return rdr_err;
error:
  if(sys && picking) {
    SL(clear_vector(result_get_buffer(&picking->result)));
  }
  goto exit;
}

enum rdr_error
rdr_pick_poll
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   size_t *count,
   const uint32_t* out_list[])
{
  void* list = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(UNLIKELY(!sys || !picking || !count || !out_list)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  SL(vector_buffer
    (result_get_buffer(&picking->result), count, NULL, NULL, &list));
  *out_list = list;

  result_swap_buffer(&picking->result);

exit:
  return rdr_err;
error:
  goto exit;
}

enum rdr_error
rdr_show_pick_buffer
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   struct rdr_world* world,
   const struct rdr_view* view,
   const uint32_t max_pick_id)
{
  struct rb_depth_stencil_desc depth_stencil_desc;
  struct rb_viewport_desc viewport_desc;
  float scale_bias[4] = { 0.f, 0.f, 0.f, 0.f };
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum { PICK_BUFFER_TEX_UNIT, DEPTH_BUFFER_TEX_UNIT };
  memset(&depth_stencil_desc, 0, sizeof(struct rb_depth_stencil_desc));
  memset(&viewport_desc, 0, sizeof(struct rb_viewport_desc));

  if(UNLIKELY(!sys || !picking || !world || !view)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  rdr_err = draw_picked_world(sys, picking, world, view);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  depth_stencil_desc.enable_depth_test = 1;
  depth_stencil_desc.enable_depth_write = 1;
  depth_stencil_desc.enable_stencil_test = 0;
  depth_stencil_desc.front_face_op.write_mask = 0;
  depth_stencil_desc.back_face_op.write_mask = 0;
  depth_stencil_desc.depth_func = RB_COMPARISON_ALWAYS;
  RBI(&sys->rb, depth_stencil(sys->ctxt, &depth_stencil_desc));

  viewport_desc.x = 0;
  viewport_desc.y = 0;
  viewport_desc.width = picking->desc.width;
  viewport_desc.height = picking->desc.height;
  viewport_desc.min_depth = 0.f;
  viewport_desc.max_depth = 1.f;
  RBI(&sys->rb, viewport(sys->ctxt, &viewport_desc));

  RBI(&sys->rb, bind_tex2d
    (sys->ctxt, picking->framebuffer.picking_tex, PICK_BUFFER_TEX_UNIT));
  RBI(&sys->rb, bind_tex2d
    (sys->ctxt, picking->framebuffer.depth_stencil_tex, DEPTH_BUFFER_TEX_UNIT));
  RBI(&sys->rb, bind_sampler
    (sys->ctxt, picking->debug.sampler, PICK_BUFFER_TEX_UNIT));
  RBI(&sys->rb, bind_sampler
    (sys->ctxt, picking->debug.sampler, DEPTH_BUFFER_TEX_UNIT));
  RBI(&sys->rb, bind_program(sys->ctxt, picking->debug.program));

  scale_bias[0] = 1.f / (float)viewport_desc.width;
  scale_bias[1] = 1.f / (float)viewport_desc.height;
  scale_bias[2] = 0.f;
  scale_bias[3] = 0.f;
  RBI(&sys->rb, uniform_data
    (picking->debug.scale_bias_uniform, 1, (void*)scale_bias));
  RBI(&sys->rb, uniform_data
    (picking->debug.max_pick_id_uniform, 1,
     (void*)(unsigned int[]){max_pick_id}));
  RBI(&sys->rb, uniform_data
    (picking->debug.pick_buffer_uniform, 1,
     (void*)(unsigned int[]){PICK_BUFFER_TEX_UNIT}));
  RBI(&sys->rb, uniform_data
    (picking->debug.depth_buffer_uniform, 1,
     (void*)(unsigned int[]){DEPTH_BUFFER_TEX_UNIT}));
  RBU(draw_geometry(&sys->rbu.quad));

  /* Reset states. */
  depth_stencil_desc.depth_func = RB_COMPARISON_LESS_EQUAL;
  RBI(&sys->rb, depth_stencil(sys->ctxt, &depth_stencil_desc));
  RBI(&sys->rb, bind_tex2d(sys->ctxt, NULL, PICK_BUFFER_TEX_UNIT));
  RBI(&sys->rb, bind_tex2d(sys->ctxt, NULL, DEPTH_BUFFER_TEX_UNIT));
  RBI(&sys->rb, bind_sampler(sys->ctxt, NULL, PICK_BUFFER_TEX_UNIT));
  RBI(&sys->rb, bind_sampler(sys->ctxt, NULL, DEPTH_BUFFER_TEX_UNIT));
  RBI(&sys->rb, bind_program(sys->ctxt, NULL));

exit:
  return rdr_err;
error:
  goto exit;
}

