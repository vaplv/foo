#ifndef RB_H
#define RB_H

#include "render_backend/rb_types.h"

/*******************************************************************************
 *
 * Render backend context.
 *
 ******************************************************************************/
extern int
rb_create_context
  (struct rb_context**);

extern int
rb_free_contex
  (struct rb_context*);

/*******************************************************************************
 *
 * Texture 2d.
 *
 ******************************************************************************/
extern int
rb_bind_tex2d
  (struct rb_context*,
   struct rb_tex2d*);

extern int
rb_create_tex2d
  (struct rb_context*,
   struct rb_tex2d**);

extern int
rb_free_tex2d
  (struct rb_context*,
   struct rb_tex2d*);

extern int
rb_tex2d_data
  (struct rb_context*,
   struct rb_tex2d*,
   const struct rb_tex2d_desc*,
   void*);

/*******************************************************************************
 *
 * Buffers.
 *
 ******************************************************************************/
extern int
rb_bind_buffer
  (struct rb_context*,
   struct rb_buffer*);

extern int
rb_buffer_data
  (struct rb_context*,
   struct rb_buffer*,
   int offset,
   int size,
   void* data);

extern int
rb_create_buffer
  (struct rb_context*,
   const struct rb_buffer_desc*,
   const void* init_data,
   struct rb_buffer**);

extern int
rb_free_buffer
  (struct rb_context*,
   struct rb_buffer*);

/*******************************************************************************
 *
 * Vertex array.
 *
 ******************************************************************************/
extern int
rb_bind_vertex_array
  (struct rb_context*,
   struct rb_vertex_array*);

extern int
rb_create_vertex_array
  (struct rb_context*,
   struct rb_vertex_array**);

extern int
rb_free_vertex_array
  (struct rb_context*,
   struct rb_vertex_array*);

extern int
rb_vertex_attrib_array
  (struct rb_context*,
   struct rb_vertex_array*,
   struct rb_buffer*,
   int count,
   const struct rb_buffer_attrib*);

extern int
rb_vertex_index_array
  (struct rb_context*,
   struct rb_vertex_array*,
   struct rb_buffer*);

/*******************************************************************************
 *
 * Shaders.
 *
 ******************************************************************************/
extern int
rb_create_shader
  (struct rb_context*,
   enum rb_shader_type,
   const char* source,
   int length, /* Do not include the null character. */
   struct rb_shader**);

extern int
rb_free_shader
  (struct rb_context*,
   struct rb_shader*);

extern int
rb_get_shader_log
  (struct rb_context*,
   struct rb_shader*,
   const char**);

extern int
rb_shader_source
  (struct rb_context*,
   struct rb_shader*,
   const char* source,
   int length);

/*******************************************************************************
 *
 * Programs.
 *
 ******************************************************************************/
extern int
rb_create_program
  (struct rb_context*,
   struct rb_program**);

extern int
rb_free_program
  (struct rb_context*,
   struct rb_program*);

extern int
rb_attach_shader
  (struct rb_context*,
   struct rb_program*,
   struct rb_shader*);

extern int
rb_detach_shader
  (struct rb_context*,
   struct rb_program*,
   struct rb_shader*);

extern int
rb_link_program
  (struct rb_context*,
   struct rb_program*);

extern int
rb_get_program_log
  (struct rb_context*,
   struct rb_program*,
   const char**);

extern int
rb_bind_program
  (struct rb_context*,
   struct rb_program*);

/*******************************************************************************
 *
 * Uniforms.
 *
 ******************************************************************************/
extern int
rb_get_uniform
  (struct rb_context*,
   struct rb_program*,
   const char* name,
   struct rb_uniform**);

extern int
rb_release_uniform
  (struct rb_context*,
   struct rb_uniform*);

extern int
rb_uniform_data
  (struct rb_context*,
   struct rb_uniform*,
   int nb,
   void*);

/*******************************************************************************
 *
 * Miscellaneous functions.
 *
 ******************************************************************************/
extern int
rb_blend
  (struct rb_context*,
   const struct rb_blend_desc*);

extern int
rb_clear
  (struct rb_context*,
   int clear_flag,
   const float color[4],
   float depth,
   char stencil);

extern int
rb_depth_stencil
  (struct rb_context*,
   const struct rb_depth_stencil_desc*);

extern int
rb_draw_indexed
  (struct rb_context*,
   enum rb_primitive_type,
   int count);

extern int
rb_flush
  (struct rb_context*);

extern int
rb_rasterizer
  (struct rb_context*,
   const struct rb_rasterizer_desc*);

extern int
rb_viewport
  (struct rb_context*,
   const struct rb_viewport_desc*);

#endif /* RB_H */

