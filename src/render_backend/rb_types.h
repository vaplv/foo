#ifndef RB_TYPES
#define RB_TYPES

#include <stddef.h>

struct mem_allocator;

/*******************************************************************************
 *
 * Render backend constants.
 *
 ******************************************************************************/
enum {
  RB_CLEAR_COLOR_BIT = 1<<0,
  RB_CLEAR_DEPTH_BIT = 1<<1,
  RB_CLEAR_STENCIL_BIT = 1<<2
};

/*******************************************************************************
 *
 * Public render backend enums.
 *
 ******************************************************************************/
enum rb_tex_format {
  RB_R,
  RB_RGB,
  RB_RGBA,
  RB_SRGB,
  RB_SRGBA
};

enum rb_tex_filter {
  RB_MIN_POINT_MAG_POINT_MIP_POINT,
  RB_MIN_LINEAR_MAG_POINT_MIP_POINT,
  RB_MIN_POINT_MAG_LINEAR_MIP_POINT,
  RB_MIN_LINEAR_MAG_LINEAR_MIP_POINT,
  RB_MIN_POINT_MAG_POINT_MIP_LINEAR,
  RB_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
  RB_MIN_POINT_MAG_LINEAR_MIP_LINEAR,
  RB_MIN_LINEAR_MAG_LINEAR_MIP_LINEAR
};

enum rb_tex_address {
  RB_ADDRESS_WRAP,
  RB_ADDRESS_CLAMP
};

enum rb_buffer_target {
  RB_BIND_VERTEX_BUFFER,
  RB_BIND_INDEX_BUFFER
};

enum rb_usage {
  RB_USAGE_DEFAULT,
  RB_USAGE_IMMUTABLE,
  RB_USAGE_DYNAMIC
};

enum rb_type {
  RB_UNKNOWN_TYPE,
  RB_FLOAT,
  RB_FLOAT2,
  RB_FLOAT3,
  RB_FLOAT4,
  RB_FLOAT4x4
};

enum rb_primitive_type {
  RB_LINE_LOOP,
  RB_TRIANGLE_LIST,
  RB_TRIANGLE_STRIP
};

enum rb_shader_type {
  RB_VERTEX_SHADER,
  RB_GEOMETRY_SHADER,
  RB_FRAGMENT_SHADER
};

enum rb_blend_func {
  RB_BLEND_ZERO,
  RB_BLEND_ONE,
  RB_BLEND_SRC_COLOR,
  RB_BLEND_ONE_MINUS_SRC_COLOR,
  RB_BLEND_DST_COLOR,
  RB_BLEND_ONE_MINUS_DST_COLOR,
  RB_BLEND_SRC_ALPHA,
  RB_BLEND_ONE_MINUS_SRC_ALPHA,
  RB_BLEND_DST_ALPHA,
  RB_BLEND_ONE_MINUS_DST_ALPHA,
  RB_BLEND_CONSTANT
};

enum rb_blend_op {
  RB_BLEND_OP_ADD,
  RB_BLEND_OP_SUB,
  RB_BLEND_OP_REVERSE_SUB,
  RB_BLEND_OP_MIN,
  RB_BLEND_OP_MAX
};

enum rb_stencil_op {
  RB_STENCIL_OP_KEEP,
  RB_STENCIL_OP_ZERO,
  RB_STENCIL_OP_REPLACE,
  RB_STENCIL_OP_INCR_SAT,
  RB_STENCIL_OP_DECR_SAT,
  RB_STENCIL_OP_INCR,
  RB_STENCIL_OP_DECR,
  RB_STENCIL_OP_INVERT
};

enum rb_comparison {
  RB_COMPARISON_NEVER,
  RB_COMPARISON_ALWAYS,
  RB_COMPARISON_LESS,
  RB_COMPARISON_EQUAL,
  RB_COMPARISON_NOT_EQUAL,
  RB_COMPARISON_LESS_EQUAL,
  RB_COMPARISON_GREATER,
  RB_COMPARISON_GREATER_EQUAL
};

enum rb_face_orientation {
  RB_ORIENTATION_CW,
  RB_ORIENTATION_CCW
};

enum rb_fill_mode {
  RB_FILL_WIREFRAME,
  RB_FILL_SOLID
};

enum rb_cull_mode {
  RB_CULL_NONE,
  RB_CULL_FRONT,
  RB_CULL_BACK
};

/*******************************************************************************
 *
 * Opaque render backend data structures.
 *
 ******************************************************************************/
struct rb_context;
struct rb_tex2d;
struct rb_buffer;
struct rb_vertex_array;
struct rb_sampler;
struct rb_shader;
struct rb_program;
struct rb_uniform;
struct rb_attrib;

/*******************************************************************************
 *
 * Public render backend data structures.
 *
 ******************************************************************************/
struct rb_config {
  size_t max_tex_size;
  size_t max_tex_max_anisotropy;
};

struct rb_sampler_desc {
  enum rb_tex_filter filter;
  enum rb_tex_address address_u;
  enum rb_tex_address address_v;
  enum rb_tex_address address_w;
  float lod_bias;
  float min_lod;
  float max_lod;
  unsigned int max_anisotropy;
};

struct rb_buffer_desc {
  size_t size;
  enum rb_buffer_target target;
  enum rb_usage usage;
};

struct rb_tex2d_desc {
  unsigned int width;
  unsigned int height;
  unsigned int mip_count;
  enum rb_tex_format format;
  enum rb_usage usage;
  int compress;
};

struct rb_buffer_attrib {
  int index;
  size_t stride;
  size_t offset;
  enum rb_type type;
};

struct rb_attrib_desc {
  const char* name;
  int index;
  enum rb_type type;
};

struct rb_uniform_desc {
  const char*name;
  enum rb_type type;
};

struct rb_viewport_desc {
  int x;
  int y;
  int width;
  int height;
  float min_depth;
  float max_depth;
};

struct rb_blend_desc {
  int enable;
  enum rb_blend_func src_blend_RGB;
  enum rb_blend_func src_blend_Alpha;
  enum rb_blend_func dst_blend_RGB;
  enum rb_blend_func dst_blend_Alpha;
  enum rb_blend_op blend_op_RGB;
  enum rb_blend_op blend_op_Alpha;
};

struct rb_depth_stencil_desc {
  int enable_depth_test;
  int enable_depth_write;
  enum rb_comparison depth_func;
  int enable_stencil_test;
  int stencil_ref;
  struct rb_stencil_op_desc {
    enum rb_stencil_op stencil_fail;
    enum rb_stencil_op depth_fail;
    enum rb_stencil_op depth_pass;
    enum rb_comparison stencil_func;
    unsigned int write_mask;
  } front_face_op, back_face_op;
};

struct rb_rasterizer_desc {
  enum rb_fill_mode fill_mode;
  enum rb_cull_mode cull_mode;
  enum rb_face_orientation front_facing;
};

#endif /* RB_TYPES */

