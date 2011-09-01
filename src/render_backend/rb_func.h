/*******************************************************************************
 *
 * Render backend context.
 *
 ******************************************************************************/
RB_FUNC(
  int,
  create_context,
    struct rb_context** out_ctxt
)

RB_FUNC(
  int,
  free_context,
    struct rb_context* ctxt
)

/*******************************************************************************
 *
 * Texture 2d.
 *
 ******************************************************************************/
RB_FUNC(
  int,
  bind_tex2d,
    struct rb_context* ctxt,
    struct rb_tex2d* tex
)

RB_FUNC(
  int,
  create_tex2d,
    struct rb_context* ctxt,
    struct rb_tex2d** out_ctxt
)

RB_FUNC(
  int,
  free_tex2d,
    struct rb_context* ctxt,
    struct rb_tex2d* tex
)

RB_FUNC(
  int,
  tex2d_data,
    struct rb_context* ctxt,
    struct rb_tex2d* tex,
    const struct rb_tex2d_desc* desc,
    void* data
)

/*******************************************************************************
 *
 * Buffers.
 *
 ******************************************************************************/
RB_FUNC(
  int,
  bind_buffer,
    struct rb_context* ctxt,
    struct rb_buffer* buf
)

RB_FUNC(
  int,
  buffer_data,
    struct rb_context* ctxt,
    struct rb_buffer* buf,
    int offset,
    int size,
    const void* data
)

RB_FUNC(
  int,
  create_buffer,
    struct rb_context* ctxt,
    const struct rb_buffer_desc* desc,
    const void* init_data,
    struct rb_buffer** out_buf
)

RB_FUNC(
  int,
  free_buffer,
    struct rb_context* ctxt,
    struct rb_buffer* buf
)

/*******************************************************************************
 *
 * Vertex array.
 *
 ******************************************************************************/
RB_FUNC(
  int,
  bind_vertex_array,
    struct rb_context* ctxt,
    struct rb_vertex_array* varray
)

RB_FUNC(
  int,
  create_vertex_array,
    struct rb_context* ctxt,
    struct rb_vertex_array** out_varray
)

RB_FUNC(
  int,
  free_vertex_array,
    struct rb_context* ctxt,
    struct rb_vertex_array* varray
)

RB_FUNC(
  int,
  remove_vertex_attrib,
    struct rb_context* ctxt,
    struct rb_vertex_array* array,
    int count,
    int* list_of_attrib_indices
)

RB_FUNC(
  int,
  vertex_attrib_array,
    struct rb_context* ctxt,
    struct rb_vertex_array* varray,
    struct rb_buffer* buf,
    int count,
    const struct rb_buffer_attrib* attr
)

RB_FUNC(
  int,
  vertex_index_array,
    struct rb_context* ctxt,
    struct rb_vertex_array* varray,
    struct rb_buffer* buf
)

/*******************************************************************************
 *
 * Shaders.
 *
 ******************************************************************************/
RB_FUNC(
  int,
  create_shader,
    struct rb_context* ctxt,
    enum rb_shader_type type,
    const char* source,
    int length, /* Do not include the null character. */
    struct rb_shader** out_shader
)

RB_FUNC(
  int,
  free_shader,
    struct rb_context* ctxt,
    struct rb_shader* shader
)

RB_FUNC(
  int,
  get_shader_log,
    struct rb_context* ctxt,
    struct rb_shader* shader,
    const char** out_log
)

RB_FUNC(
  int,
  is_shader_attached,
    struct rb_context* ctxt,
    struct rb_shader* shader,
    int* out_is_attached
)

RB_FUNC(
  int,
  shader_source,
    struct rb_context* ctxt,
    struct rb_shader* shader,
    const char* source,
    int length /* Do not include the null character. */
)

/*******************************************************************************
 *
 * Programs.
 *
 ******************************************************************************/
RB_FUNC(
  int,
  attach_shader,
    struct rb_context* ctxt,
    struct rb_program* prog,
    struct rb_shader* shader
)

RB_FUNC(
  int,
  bind_program,
    struct rb_context* ctxt,
    struct rb_program* prog
)

RB_FUNC(
  int,
  create_program,
    struct rb_context* ctxt,
    struct rb_program** out_prog
)

RB_FUNC(
  int,
  detach_shader,
    struct rb_context* ctxt,
    struct rb_program* prog,
    struct rb_shader* shader
)

RB_FUNC(
  int,
  free_program,
    struct rb_context* ctxt,
    struct rb_program* prog
)

RB_FUNC(
  int,
  get_program_log,
    struct rb_context* ctxt,
    struct rb_program* prog,
    const char** out_log
)

RB_FUNC(
  int,
  link_program,
    struct rb_context* ctxt,
    struct rb_program* prog
)

/*******************************************************************************
 *
 * Program uniforms.
 *
 ******************************************************************************/
RB_FUNC(
  int,
  get_named_uniform,
    struct rb_context* ctxt,
    struct rb_program* prog,
    const char* name,
    struct rb_uniform** out_var
)

RB_FUNC(
  int,
  get_uniforms,
    struct rb_context* ctxt,
    struct rb_program* prog,
    size_t* out_nb_uniforms,
    struct rb_uniform* out_uniform_list[]
)

RB_FUNC(
  int,
  get_uniform_desc,
    struct rb_context* ctxt,
    struct rb_uniform* prog,
    struct rb_uniform_desc* out_desc
)

RB_FUNC(
  int,
  release_uniforms,
    struct rb_context* ctxt,
    size_t nb_uniforms,
    struct rb_uniform* uniform_list[]
)

RB_FUNC(
  int,
  uniform_data,
    struct rb_context* ctxt,
    struct rb_uniform* var,
    int count,
    const void* data
)

/*******************************************************************************
 *
 * Program attributes.
 *
 ******************************************************************************/
RB_FUNC(
  int,
  get_attribs,
    struct rb_context* ctxt,
    struct rb_program* prog,
    size_t* out_nb_attribs,
    struct rb_attrib* out_attrib_list[]
)

RB_FUNC(
  int,
  get_named_attrib,
    struct rb_context* ctxt,
    struct rb_program* prog,
    const char* name,
    struct rb_attrib** out_attrib
)

RB_FUNC(
  int,
  release_attribs,
    struct rb_context* ctxt,
    size_t nb_attribs,
    struct rb_attrib* attrib_list[]
)

RB_FUNC(
  int,
  attrib_data,
    struct rb_context* ctxt,
    struct rb_attrib* attr,
    const void* data
)

RB_FUNC(
  int,
  get_attrib_desc,
    struct rb_context* ctxt,
    const struct rb_attrib* attr,
    struct rb_attrib_desc* attr_desc
)

/*******************************************************************************
 *
 * Miscellaneous functions.
 *
 ******************************************************************************/
RB_FUNC(
  int,
  blend,
    struct rb_context* ctxt,
    const struct rb_blend_desc* desc
)

RB_FUNC(
  int,
  clear,
    struct rb_context* ctxt,
    int clear_flag,
    const float color_val[4],
    float depth_val,
    char stencil_val
)

RB_FUNC(
  int,
  depth_stencil,
    struct rb_context* ctxt,
    const struct rb_depth_stencil_desc* desc
)

RB_FUNC(
  int,
  draw_indexed,
    struct rb_context* ctxt,
    enum rb_primitive_type prim_type,
    int count
)

RB_FUNC(
  int,
  flush,
    struct rb_context* ctxt
)

RB_FUNC(
  int,
  rasterizer,
    struct rb_context* ctxt,
    const struct rb_rasterizer_desc* desc
)

RB_FUNC(
  int,
  viewport,
    struct rb_context* ctxt,
    const struct rb_viewport_desc* desc
)

