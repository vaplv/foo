#include "renderer/regular/rdr_attrib_c.h"
#include "renderer/regular/rdr_draw_desc.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_picking.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/regular/rdr_uniform.h"
#include "renderer/regular/rdr_world_c.h"
#include "render_backend/rbi.h"
#include "renderer/rdr.h"
#include "renderer/rdr_world.h"
#include "stdlib/sl.h"
#include "stdlib/sl_vector.h"
#include "sys/math.h"
#include <assert.h>
#include <string.h>

enum { PICK_UNIFORM_MVP, PICK_UNIFORM_MDL_ID, NB_PICK_UNIFORMS };

struct rdr_picking {
  struct rdr_picking_desc desc;
  struct result {
    struct sl_vector* picked_model_instance_list; /* vector of draw id */
  } result;
  struct framebuffer {
    struct rb_framebuffer* buffer;
    struct rb_tex2d* picking_tex; /* TODO replace by a render buffer. */
    struct rb_tex2d* depth_stencil_tex; /* TODO replace by a render buffer. */
  } framebuffer;
  struct shading {
    struct rb_program* program;
    struct rb_shader* vertex_shader;
    struct rb_shader* fragment_shader;
    struct rdr_uniform uniform_list[NB_PICK_UNIFORMS];
  } shading;
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

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
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
  tex2ddesc.format = RB_R_UINT16;
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
  shading->uniform_list[PICK_UNIFORM_MDL_ID].usage = RDR_DRAW_ID_UNIFORM;
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
release_result(struct rdr_system* sys UNUSED, struct result* result)
{
  assert(sys && result);
  if(result->picked_model_instance_list)
    SL(free_vector(result->picked_model_instance_list));
  memset(result, 0, sizeof(struct result));
}

static enum rdr_error
init_result(struct rdr_system* sys, struct result* result)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  assert(sys && result);

  sl_err = sl_create_vector
    (sizeof(uint32_t),
     16,
     sys->allocator,
     &result->picked_model_instance_list);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }
exit:
  return rdr_err;
error:
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
  rdr_err = init_framebuffer
    (sys, desc->width, desc->height, &picking->framebuffer);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = init_shading(sys, &picking->shading);
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
  release_result(sys, &picking->result);
  MEM_FREE(sys->allocator, picking);
  return RDR_NO_ERROR;
}

extern enum rdr_error
rdr_pick
  (struct rdr_system* sys,
   struct rdr_picking* picking,
   struct rdr_world* world,
   const struct rdr_view* view,
   const unsigned int pos[2],
   const unsigned int size[2],
   enum rdr_pick pick_type)
{
  struct rdr_draw_desc draw_desc;
  struct rdr_view pick_view;
  float scale[2] = {0.f, 0.f}; /* scale factor from viewport to pick space. */
  unsigned int viewport_pos[2] = {0, 0}; /* position in viewport space. */
  unsigned int coord0[2] = {0, 0}; /* blit coordinate 0 */
  unsigned int coord1[2] = {0, 0}; /* blit coordinate 1 */
  enum rdr_error rdr_err = RDR_NO_ERROR;
  memset(&draw_desc, 0, sizeof(struct rdr_draw_desc));

  if(UNLIKELY(!picking || !world || !view || !pos || !size)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  if(pick_type == RDR_PICK_NONE
  || pos[0] < view->x 
  || pos[1] < view->y 
  || size[0] == 0
  || size[1] == 0)
    goto exit;

  /* Adjust the input viewport to match the pick definition. */
  memcpy(&pick_view, view, sizeof(struct rdr_view));
  pick_view.x = pick_view.y = 0;
  pick_view.width = picking->desc.width;
  pick_view.height = picking->desc.height;

  /* Compute the blit coordinates in pick space. */
  scale[0] = (float)picking->desc.width / (float)(view->width - view->x);
  scale[1] = (float)picking->desc.height / (float)(view->height - view->y);
  viewport_pos[0] = pos[0] - view->x; 
  viewport_pos[1] = pos[1] - view->y;
  coord0[0] = viewport_pos[0] * scale[0];
  coord0[1] = viewport_pos[1] * scale[1];
  coord1[0] = (viewport_pos[0] + size[0] - 1) * scale[0];
  coord1[1] = (viewport_pos[1] + size[1] - 1) * scale[1];
  coord1[0] = MIN(coord1[0], picking->desc.width);
  coord1[1] = MIN(coord1[1], picking->desc.height);

  /* Define the draw desc. */
  if(pick_type == RDR_PICK_MODEL_INSTANCE) {
    draw_desc.uniform_list = picking->shading.uniform_list;
    draw_desc.nb_uniforms = NB_PICK_UNIFORMS;
    draw_desc.draw_id_bias = 0;
    draw_desc.bind_flag = RDR_BIND_ATTRIB_POSITION;
  }

  /* Draw the world into the picking buffer. */
  RBI(&sys->rb, bind_framebuffer(sys->ctxt, picking->framebuffer.buffer));
  RBI(&sys->rb, bind_program(sys->ctxt, picking->shading.program));
  RDR(draw_world(world, &pick_view, &draw_desc));
  RBI(&sys->rb, bind_framebuffer(sys->ctxt, NULL));
  RBI(&sys->rb, bind_program(sys->ctxt, NULL));

  /* Read back pick texture content. */
  SL(clear_vector(picking->result.picked_model_instance_list));
  SL(vector_push_back_n
    (picking->result.picked_model_instance_list, 
     (coord1[0] - coord0[0]) * (coord1[1] - coord0[0]), 
     (uint32_t[]){0}));
  /* TODO */

exit:
  return rdr_err;
error:
  goto exit;
}


