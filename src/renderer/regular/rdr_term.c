#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_font_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/regular/rdr_term_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_font.h"
#include "renderer/rdr_system.h"
#include "renderer/rdr_term.h"
#include "stdlib/sl.h"
#include "stdlib/sl_vector.h"
#include "stdlib/sl_wstring.h"
#include "sys/list.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <float.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define TAB_WIDTH 4

#define VERTICES_PER_GLYPH 4
#define INDICES_PER_GLYPH 6
#define SIZEOF_GLYPH_VERTEX (3/*pos*/ + 2/*tex*/ + 3/*col*/) * sizeof(float)
#define GLYPH_CACHE_TEX_UNIT 0
#define POS_ATTRIB_ID 0
#define TEX_ATTRIB_ID 1
#define COL_ATTRIB_ID 2
#define NB_ATTRIBS 3

struct line {
  struct list_node node;
  struct sl_wstring* string;
  struct sl_vector* color_list; /* Vector of struct rgb. */
};

struct screen {
  struct list_node free_line_list;
  struct list_node stdout_line_list;
  struct line* outbuf;
  struct line* cmdbuf;
  struct line* line_list;
  size_t nb_lines;
  size_t lines_per_screen;
  size_t scroll_id;
  size_t cursor;
};

struct printer {
  struct text {
    struct rb_buffer_attrib attrib_list[NB_ATTRIBS];
    struct rb_buffer* glyph_vertex_buffer;
    struct rb_buffer* glyph_index_buffer;
    struct rb_vertex_array* varray;
    struct rb_shader* vertex_shader;
    struct rb_shader* fragment_shader;
    struct rb_program* shading_program;
    struct rb_sampler* sampler;
    struct rb_uniform* sampler_uniform;
    struct rb_uniform* scale_uniform;
    struct rb_uniform* bias_uniform;
    size_t max_nb_glyphs; /* Maximum number glyphs that the printer can draw. */
    size_t nb_glyphs; /* Number of glyphs currently drawn by the printer. */
  } text;
  struct cursor {
    struct rb_buffer* vertex_buffer;
    struct rb_vertex_array* varray;
    struct rb_shader* vertex_shader;
    struct rb_shader* fragment_shader;
    struct rb_program* shading_program;
    struct rb_uniform* scale_bias_uniform;
    size_t y;
  } cursor;
};

/* Minimal blob data structure. */
struct blob {
  struct mem_allocator* allocator;
  unsigned char* buffer;
  size_t size;
  size_t id;
};

struct rdr_term {
  struct ref ref;
  struct rdr_font* font;
  struct rdr_system* sys;
  size_t width; /* In pixels. */
  size_t height; /* In Pixels. */
  struct blob scratch;
  struct screen screen;
  struct printer printer;
};

/*******************************************************************************
 *
 * Embedded shader sources.
 *
 ******************************************************************************/
static const char* print_vs_source =
  "#version 330\n"
  "layout(location =" STR(POS_ATTRIB_ID) ") in vec3 pos;\n"
  "layout(location =" STR(TEX_ATTRIB_ID) ") in vec2 tex;\n"
  "layout(location =" STR(COL_ATTRIB_ID) ") in vec3 col;\n"
  "uniform vec3 scale;\n"
  "uniform vec3 bias;\n"
  "smooth out vec2 glyph_tex;\n"
  "flat   out vec3 glyph_col;\n"
  "void main()\n"
  "{\n"
  "  glyph_tex = tex;\n"
  "  glyph_col = col;\n"
  "  gl_Position = vec4(pos * scale + bias, 1.f);\n"
  "}\n";

static const char* print_fs_source =
  "#version 330\n"
  "uniform sampler2D glyph_cache;\n"
  "smooth in vec2 glyph_tex;\n"
  "flat   in vec3 glyph_col;\n"
  "out vec4 color;\n"
  "void main()\n"
  "{\n"
  "  float val = texture(glyph_cache, glyph_tex).r;\n"
  "  color = vec4(val * glyph_col, val);\n"
  "}\n";

static const char* cursor_vs_source =
  "#version 330\n"
  "layout(location = 0) in vec2 pos;\n"
  "uniform vec4 scale_bias;\n"
  "void main()\n"
  "{\n"
  " gl_Position = vec4(pos * scale_bias.xy + scale_bias.zw, vec2(0.f, 1.f));\n"
  "}\n";

static const char* cursor_fs_source =
  "#version 330\n"
  "out vec4 color;\n"
  "void main()\n"
  "{\n"
  " color = vec4(1.f);\n"
  "}\n";

/*******************************************************************************
 *
 * Minimal implementation of a growable blob buffer.
 *
 ******************************************************************************/
static enum rdr_error
blob_reserve(struct blob* blob, size_t size)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  assert(blob && blob->allocator);

  if(size > blob->size) {
    if(blob->buffer) {
      blob->buffer = MEM_REALLOC(blob->allocator, blob->buffer, size);
    } else {
      blob->buffer = MEM_ALLOC(blob->allocator, size);
    }
    if(!blob->buffer) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }
    blob->size = size;
  }

exit:
  return rdr_err;
error:
  if(blob->buffer) {
    MEM_FREE(blob->allocator, blob->buffer);
    blob->buffer = NULL;
  }
  blob->size = 0;
  blob->id = 0;
  goto exit;
}

static void
blob_init(struct mem_allocator* allocator, struct blob* blob)
{
  assert(allocator && blob);
  memset(blob, 0, sizeof(struct blob));
  blob->allocator = allocator;
}


static void
blob_release(struct blob* blob)
{
  assert(blob && blob->allocator);
  MEM_FREE(blob->allocator, blob->buffer);
  memset(blob, 0, sizeof(struct blob));
}

static FINLINE void
blob_clear(struct blob* blob)
{
  assert(blob);
  blob->id = 0;
}

static FINLINE void*
blob_buffer(struct blob* blob)
{
  assert(blob);
  return blob->buffer;
}

static FINLINE size_t
blob_id(struct blob* blob)
{
  assert(blob);
  return blob->id;
}

static enum rdr_error
blob_push_back(struct blob* blob, const void* data, size_t size)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  assert(blob && data);

  if(blob->id + size > blob->size) {
    rdr_err = blob_reserve(blob, blob->id + size);
    if(rdr_err != RDR_NO_ERROR)
      goto error;
  }
  memcpy(blob->buffer + blob->id, data, size);
  blob->id += size;
exit:
  return rdr_err;
error:
  goto exit;
}

/*******************************************************************************
 *
 * Screen functions.
 *
 ******************************************************************************/
static FINLINE void
release_line(struct screen* scr, struct line* line)
{
  assert(scr && line);
  list_move(&line->node, &scr->free_line_list);
}

static void
new_buf(struct screen* scr, enum rdr_term_output output)
{
  struct line** line = NULL;
  struct list_node* node = NULL;
  assert(scr);

  if(!scr->line_list)
    return;

  if(output == RDR_TERM_STDOUT) {
    line = &scr->outbuf;
  } else {
    line = &scr->cmdbuf;
    scr->cursor = 0;
  }
  if(*line) {
    list_add(&scr->stdout_line_list, &(*line)->node);
    *line = NULL;
  }
  if(is_list_empty(&scr->free_line_list)) {
    node = list_tail(&scr->stdout_line_list);
  } else {
    node = list_head(&scr->free_line_list);
  }
  list_del(node);
  (*line) = CONTAINER_OF(node, struct line, node);
  SL(clear_wstring((*line)->string));
  SL(clear_vector((*line)->color_list));
}

static void
reset_screen(struct rdr_system* sys, struct screen* scr)
{
  size_t i = 0;
  assert(scr);

  for(i = 0; i < scr->nb_lines; ++i) {
    struct line* line = scr->line_list + i;
    if(line->string)
      SL(free_wstring(line->string));
    if(line->color_list)
      SL(free_vector(line->color_list));
  }
  if(scr->line_list) {
    MEM_FREE(sys->allocator, scr->line_list);
    scr->line_list = NULL;
  }
  memset(scr, 0, sizeof(struct screen));
  list_init(&scr->free_line_list);
  list_init(&scr->stdout_line_list);
}

static enum rdr_error
init_screen(struct rdr_system* sys UNUSED, struct screen* scr)
{
  assert(scr);
  memset(scr, 0, sizeof(struct screen));
  list_init(&scr->free_line_list);
  list_init(&scr->stdout_line_list);
  return RDR_NO_ERROR;
}

static enum rdr_error
screen_storage
  (struct rdr_system* sys,
   struct screen* scr,
   size_t lines_per_screen)
{
  /* Arbitrary set the max number of saved lines to 4 times the number of
   * printed lines per screen. */
  const size_t total_nb_lines = lines_per_screen * 4;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  assert(sys && scr);

  reset_screen(sys, scr);
  scr->lines_per_screen = lines_per_screen;
  scr->scroll_id = 0;

  if(!lines_per_screen)
    goto exit;

  scr->nb_lines = total_nb_lines;
  scr->line_list = MEM_ALLOC
    (sys->allocator, total_nb_lines * sizeof(struct line));
  if(!scr->line_list) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  for(i = 0; i < total_nb_lines; ++i) {
    struct line* line = scr->line_list + i;
    enum sl_error sl_err = SL_NO_ERROR;
    list_init(&line->node);
    sl_err = sl_create_wstring(NULL, sys->allocator, &line->string);
    if(SL_NO_ERROR != sl_err) {
      rdr_err = sl_to_rdr_error(sl_err);
      goto error;
    }
    sl_err = sl_create_vector
      (sizeof(unsigned char) * 3,
       ALIGNOF(unsigned char),
       sys->allocator,
       &line->color_list);
    if(SL_NO_ERROR != sl_err) {
      rdr_err = sl_to_rdr_error(sl_err);
      goto error;
    }
    list_add(&scr->free_line_list, &line->node);
  }

exit:
  return rdr_err;
error:
  reset_screen(sys, scr);
  goto exit;
}

/*******************************************************************************
 *
 * Printer functions.
 *
 ******************************************************************************/
static void
printer_storage
  (struct rdr_system* sys,
   struct printer* printer,
   size_t nb_glyphs,
   int min_glyph_pos_y,
   struct blob* scratch)
{
  struct rb_buffer_desc buffer_desc;
  memset(&buffer_desc, 0, sizeof(struct rb_buffer_desc));
  assert(sys && printer);

  printer->cursor.y = min_glyph_pos_y;

  if(printer->text.glyph_vertex_buffer) {
    RBI(&sys->rb, buffer_ref_put(printer->text.glyph_vertex_buffer));
    printer->text.glyph_vertex_buffer = NULL;
  }
  if(printer->text.glyph_index_buffer) {
    RBI(&sys->rb, buffer_ref_put(printer->text.glyph_index_buffer));
    printer->text.glyph_index_buffer = NULL;
  }

  printer->text.max_nb_glyphs = nb_glyphs;
  if(0 == nb_glyphs) {
    const int attrib_id_list[NB_ATTRIBS] = {
      [0] = POS_ATTRIB_ID,
      [1] = TEX_ATTRIB_ID,
      [2] = COL_ATTRIB_ID
    };
    STATIC_ASSERT(3 == NB_ATTRIBS, Unexpected_constant);
    RBI(&sys->rb, remove_vertex_attrib
      (printer->text.varray, NB_ATTRIBS, attrib_id_list));
  } else {
    const size_t vertex_bufsiz =
      nb_glyphs * VERTICES_PER_GLYPH * SIZEOF_GLYPH_VERTEX;
    const size_t index_bufsiz =
      nb_glyphs * INDICES_PER_GLYPH * sizeof(unsigned int);
    unsigned int i = 0;

    blob_reserve
      (scratch, vertex_bufsiz > index_bufsiz ? vertex_bufsiz : index_bufsiz);

    /* Create the vertex buffer. The vertex buffer data are going to be filled
     * by the draw function with respect to the printed terminal lines. */
    buffer_desc.size = vertex_bufsiz;
    buffer_desc.target = RB_BIND_VERTEX_BUFFER;
    buffer_desc.usage = RB_USAGE_DYNAMIC;
    RBI(&sys->rb, create_buffer
      (sys->ctxt, &buffer_desc, NULL, &printer->text.glyph_vertex_buffer));

    /* Create the immutable index buffer. Its internal data are the indices of
     * the ordered glyphs of the vertex buffer. */
    buffer_desc.size = index_bufsiz;
    buffer_desc.target = RB_BIND_INDEX_BUFFER;
    buffer_desc.usage = RB_USAGE_IMMUTABLE;
    blob_clear(scratch);
    for(i = 0; i < nb_glyphs * VERTICES_PER_GLYPH; i += VERTICES_PER_GLYPH) {
      const unsigned int indices[INDICES_PER_GLYPH] = {
        0+i, 1+i, 3+i, 3+i, 1+i, 2+i
      };
      blob_push_back(scratch, indices, sizeof(indices));
    }
    RBI(&sys->rb, create_buffer
      (sys->ctxt,
       &buffer_desc,
       blob_buffer(scratch),
       &printer->text.glyph_index_buffer));

    /* Setup the vertex array. */
    RBI(&sys->rb, vertex_attrib_array
      (printer->text.varray,
       printer->text.glyph_vertex_buffer,
       NB_ATTRIBS,
       printer->text.attrib_list));
    RBI(&sys->rb, vertex_index_array
      (printer->text.varray, printer->text.glyph_index_buffer));
  }
}

static void
printer_data
  (struct rdr_system* sys,
   struct printer* printer,
   size_t nb_glyphs,
   float* data)
{
  const size_t size = nb_glyphs * VERTICES_PER_GLYPH * SIZEOF_GLYPH_VERTEX;
  assert(sys && printer && (!nb_glyphs || data));
  assert
    (  size
    <= (printer->text.max_nb_glyphs*VERTICES_PER_GLYPH*SIZEOF_GLYPH_VERTEX));

  RBI(&sys->rb, buffer_data(printer->text.glyph_vertex_buffer, 0, size, data));
  printer->text.nb_glyphs = nb_glyphs;
}

static void
printer_draw_cursor
  (struct rdr_system* sys,
   struct rdr_font* font,
   struct cursor* cursor,
   size_t width,
   size_t height,
   size_t cursor_x,
   size_t cursor_y,
   size_t cursor_width)
{
  struct rb_blend_desc blend_desc;
  struct rdr_font_metrics metrics;
  float scale_bias[4];
  const float two_rcp_width = 2.f/(float)width;
  const float two_rcp_height = 2.f/(float)height;
  memset(&blend_desc, 0, sizeof(blend_desc));
  assert(sys && font && cursor);

  RDR(get_font_metrics(font, &metrics));
  scale_bias[0] = cursor_width * two_rcp_width;
  scale_bias[1] = metrics.line_space * two_rcp_height;
  scale_bias[2] = cursor_x * two_rcp_width - 1.f;
  scale_bias[3] = (cursor_y + metrics.min_glyph_pos_y) * two_rcp_height - 1.f;

  blend_desc.enable = 1;
  blend_desc.src_blend_RGB = RB_BLEND_ONE;
  blend_desc.src_blend_Alpha = RB_BLEND_ZERO;
  blend_desc.dst_blend_RGB = RB_BLEND_DST_ALPHA;
  blend_desc.dst_blend_Alpha = RB_BLEND_ONE;
  blend_desc.blend_op_RGB = RB_BLEND_OP_SUB;
  blend_desc.blend_op_Alpha = RB_BLEND_OP_SUB;
  RBI(&sys->rb, blend(sys->ctxt, &blend_desc));

  RBI(&sys->rb, bind_program(sys->ctxt, cursor->shading_program));
  RBI(&sys->rb, uniform_data(cursor->scale_bias_uniform, 1, (void*)scale_bias));
  RBI(&sys->rb, bind_vertex_array(sys->ctxt, cursor->varray));

  RBI(&sys->rb, draw(sys->ctxt, RB_TRIANGLE_STRIP, 4));

  blend_desc.enable = 0;
  RBI(&sys->rb, blend(sys->ctxt, &blend_desc));
  RBI(&sys->rb, bind_program(sys->ctxt, NULL));
  RBI(&sys->rb, bind_vertex_array(sys->ctxt, NULL));
}

static void
printer_draw_text
  (struct rdr_system* sys,
   struct rdr_font* font,
   struct text* text,
   size_t width,
   size_t height)
{
  const float scale[3] = { 2.f/(float)width, 2.f/(float)height, 1.f };
  const float bias[3] = { -1.f, -1.f, 0.f };
  struct rb_blend_desc blend_desc;
  struct rb_tex2d* glyph_cache = NULL;
  memset(&blend_desc, 0, sizeof(blend_desc));
  assert(sys && font && text);

  blend_desc.enable = 1;
  blend_desc.src_blend_RGB = RB_BLEND_SRC_ALPHA;
  blend_desc.src_blend_Alpha = RB_BLEND_ONE;
  blend_desc.dst_blend_RGB = RB_BLEND_ONE_MINUS_SRC_ALPHA;
  blend_desc.dst_blend_Alpha = RB_BLEND_ZERO;
  blend_desc.blend_op_RGB = RB_BLEND_OP_ADD;
  blend_desc.blend_op_Alpha = RB_BLEND_OP_ADD;
  RBI(&sys->rb, blend(sys->ctxt, &blend_desc));

  RDR(get_font_texture(font, &glyph_cache));

  RBI(&sys->rb, bind_tex2d(sys->ctxt, glyph_cache, GLYPH_CACHE_TEX_UNIT));
  RBI(&sys->rb, bind_sampler(sys->ctxt, text->sampler, GLYPH_CACHE_TEX_UNIT));

  RBI(&sys->rb, bind_program(sys->ctxt, text->shading_program));
  RBI(&sys->rb, uniform_data
    (text->sampler_uniform, 1, (void*)(int[]){GLYPH_CACHE_TEX_UNIT}));
  RBI(&sys->rb, uniform_data(text->scale_uniform, 1, (void*)scale));
  RBI(&sys->rb, uniform_data(text->bias_uniform, 1, (void*)bias));

  RBI(&sys->rb, bind_vertex_array(sys->ctxt, text->varray));
  RBI(&sys->rb, draw_indexed
    (sys->ctxt, RB_TRIANGLE_LIST, text->nb_glyphs * INDICES_PER_GLYPH));

  blend_desc.enable = 0;
  RBI(&sys->rb, blend(sys->ctxt, &blend_desc));
  RBI(&sys->rb, bind_program(sys->ctxt, NULL));
  RBI(&sys->rb, bind_vertex_array(sys->ctxt, NULL));
  RBI(&sys->rb, bind_tex2d(sys->ctxt, NULL, GLYPH_CACHE_TEX_UNIT));
  RBI(&sys->rb, bind_sampler(sys->ctxt, NULL, GLYPH_CACHE_TEX_UNIT));
}

static void
printer_draw
  (struct rdr_system* sys,
   struct rdr_font* font,
   struct printer* printer,
   size_t width,
   size_t height,
   size_t cursor_x,
   size_t cursor_y,
   size_t cursor_width)
{
  struct rb_depth_stencil_desc depth_stencil_desc;
  struct rb_viewport_desc viewport_desc;
  memset(&depth_stencil_desc, 0, sizeof(depth_stencil_desc));
  memset(&viewport_desc, 0, sizeof(viewport_desc));
  assert(sys && printer);
  assert(width < INT_MAX && height < INT_MAX);

  depth_stencil_desc.enable_depth_test = 0;
  depth_stencil_desc.enable_depth_write = 0;
  depth_stencil_desc.enable_stencil_test = 0;
  depth_stencil_desc.front_face_op.write_mask = 0;
  depth_stencil_desc.back_face_op.write_mask = 0;
  RBI(&sys->rb, depth_stencil(sys->ctxt, &depth_stencil_desc));

  viewport_desc.x = 0;
  viewport_desc.y = 0;
  viewport_desc.width = (int)width;
  viewport_desc.height = (int)height;
  viewport_desc.min_depth = 0.f;
  viewport_desc.max_depth = 1.f;
  RBI(&sys->rb, viewport(sys->ctxt, &viewport_desc));

  printer_draw_text(sys, font, &printer->text, width, height);
  printer_draw_cursor
    (sys,
     font,
     &printer->cursor,
     width,
     height,
     cursor_x,
     cursor_y,
     cursor_width);
}

static void
shutdown_printer_cursor(struct rdr_system* sys, struct cursor* cursor)
{
  struct rb_context* ctxt = NULL;
  assert(sys && cursor);

  ctxt = sys->ctxt;

  if(cursor->vertex_buffer)
    RBI(&sys->rb, buffer_ref_put(cursor->vertex_buffer));
  if(cursor->varray)
    RBI(&sys->rb, vertex_array_ref_put(cursor->varray));

  if(cursor->shading_program)
    RBI(&sys->rb, program_ref_put(cursor->shading_program));
  if(cursor->vertex_shader)
    RBI(&sys->rb, shader_ref_put(cursor->vertex_shader));
  if(cursor->fragment_shader)
    RBI(&sys->rb, shader_ref_put(cursor->fragment_shader));

  if(cursor->scale_bias_uniform)
    RBI(&sys->rb, uniform_ref_put(cursor->scale_bias_uniform));
}

static void
shutdown_printer_text(struct rdr_system* sys, struct text* text)
{
  struct rb_context* ctxt = NULL;
  assert(sys && text);

  ctxt = sys->ctxt;

  if(text->glyph_vertex_buffer)
    RBI(&sys->rb, buffer_ref_put(text->glyph_vertex_buffer));
  if(text->glyph_index_buffer)
    RBI(&sys->rb, buffer_ref_put(text->glyph_index_buffer));
  if(text->varray)
    RBI(&sys->rb, vertex_array_ref_put(text->varray));

  if(text->shading_program)
    RBI(&sys->rb, program_ref_put(text->shading_program));
  if(text->vertex_shader)
    RBI(&sys->rb, shader_ref_put(text->vertex_shader));
  if(text->fragment_shader)
    RBI(&sys->rb, shader_ref_put(text->fragment_shader));

  if(text->sampler)
    RBI(&sys->rb, sampler_ref_put(text->sampler));

  if(text->sampler_uniform)
    RBI(&sys->rb, uniform_ref_put(text->sampler_uniform));
  if(text->scale_uniform)
    RBI(&sys->rb, uniform_ref_put(text->scale_uniform));
  if(text->bias_uniform)
    RBI(&sys->rb, uniform_ref_put(text->bias_uniform));
}

static void
shutdown_printer(struct rdr_system* sys, struct printer* printer)
{
  shutdown_printer_text(sys, &printer->text);
  shutdown_printer_cursor(sys, &printer->cursor);
}

static void
init_printer_cursor(struct rdr_system* sys, struct cursor* cursor)
{
  const float vertices[] = { 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f, 0.f };
  struct rb_buffer_desc buffer_desc;
  struct rb_buffer_attrib attrib;
  memset(&buffer_desc, 0, sizeof(struct rb_buffer_desc));
  memset(&attrib, 0, sizeof(struct rb_buffer_attrib));
  assert(sys && cursor);

  /* Vertex buffer. */
  buffer_desc.size = sizeof(vertices);
  buffer_desc.target = RB_BIND_VERTEX_BUFFER;
  buffer_desc.usage = RB_USAGE_IMMUTABLE;
  RBI(&sys->rb, create_buffer
    (sys->ctxt, &buffer_desc, vertices, &cursor->vertex_buffer));

  /* Vertex array. */
  attrib.index = 0;
  attrib.stride = 2 * sizeof(float);
  attrib.offset = 0;
  attrib.type = RB_FLOAT2;
  RBI(&sys->rb, create_vertex_array(sys->ctxt, &cursor->varray));
  RBI(&sys->rb, vertex_attrib_array
    (cursor->varray, cursor->vertex_buffer, 1, &attrib));

  /* Shaders. */
  RBI(&sys->rb, create_shader
    (sys->ctxt,
     RB_VERTEX_SHADER,
     cursor_vs_source,
     strlen(cursor_vs_source),
     &cursor->vertex_shader));
  RBI(&sys->rb, create_shader
    (sys->ctxt,
     RB_FRAGMENT_SHADER,
     cursor_fs_source,
     strlen(cursor_fs_source),
     &cursor->fragment_shader));

  /* Shading program. */
  RBI(&sys->rb, create_program(sys->ctxt, &cursor->shading_program));
  RBI(&sys->rb, attach_shader(cursor->shading_program, cursor->vertex_shader));
  RBI(&sys->rb, attach_shader(cursor->shading_program, cursor->fragment_shader));
  RBI(&sys->rb, link_program(cursor->shading_program));

  RBI(&sys->rb, get_named_uniform
    (sys->ctxt,
     cursor->shading_program,
     "scale_bias",
     &cursor->scale_bias_uniform));
}

static void
init_printer_text(struct rdr_system* sys, struct text* text)
{
  struct rb_sampler_desc sampler_desc;
  struct rb_context* ctxt = NULL;
  assert(sys && text);

  ctxt = sys->ctxt;

  /* Vertex array. */
  text->attrib_list[0].index = POS_ATTRIB_ID; /* Position .*/
  text->attrib_list[0].stride = SIZEOF_GLYPH_VERTEX;
  text->attrib_list[0].offset = 0;
  text->attrib_list[0].type = RB_FLOAT3;
  text->attrib_list[1].index = TEX_ATTRIB_ID; /* Tex coord. */
  text->attrib_list[1].stride = SIZEOF_GLYPH_VERTEX;
  text->attrib_list[1].offset = 3 * sizeof(float);
  text->attrib_list[1].type = RB_FLOAT2;
  text->attrib_list[2].index = COL_ATTRIB_ID; /* Color. */
  text->attrib_list[2].stride = SIZEOF_GLYPH_VERTEX;
  text->attrib_list[2].offset = 5 * sizeof(float);
  text->attrib_list[2].type = RB_FLOAT3;
  RBI(&sys->rb, create_vertex_array(ctxt, &text->varray));

  /* Sampler. */
  sampler_desc.filter = RB_MIN_POINT_MAG_POINT_MIP_POINT;
  sampler_desc.address_u = RB_ADDRESS_CLAMP;
  sampler_desc.address_v = RB_ADDRESS_CLAMP;
  sampler_desc.address_w = RB_ADDRESS_CLAMP;
  sampler_desc.lod_bias = 0;
  sampler_desc.min_lod = -FLT_MAX;
  sampler_desc.max_lod = FLT_MAX;
  sampler_desc.max_anisotropy = 1;
  RBI(&sys->rb, create_sampler(ctxt, &sampler_desc, &text->sampler));

  /* Shaders. */
  RBI(&sys->rb, create_shader
    (ctxt,
     RB_VERTEX_SHADER,
     print_vs_source,
     strlen(print_vs_source),
     &text->vertex_shader));
  RBI(&sys->rb, create_shader
    (ctxt,
     RB_FRAGMENT_SHADER,
     print_fs_source,
     strlen(print_fs_source),
     &text->fragment_shader));

  /* Shading program. */
  RBI(&sys->rb, create_program(ctxt, &text->shading_program));
  RBI(&sys->rb, attach_shader(text->shading_program, text->vertex_shader));
  RBI(&sys->rb, attach_shader(text->shading_program, text->fragment_shader));
  RBI(&sys->rb, link_program(text->shading_program));

  RBI(&sys->rb, get_named_uniform
    (ctxt, text->shading_program, "glyph_cache", &text->sampler_uniform));
  RBI(&sys->rb, get_named_uniform
    (ctxt, text->shading_program, "scale", &text->scale_uniform));
  RBI(&sys->rb, get_named_uniform
    (ctxt, text->shading_program, "bias", &text->bias_uniform));
}

static enum rdr_error
init_printer(struct rdr_system* sys, struct printer* printer)
{
  init_printer_text(sys, &printer->text);
  init_printer_cursor(sys, &printer->cursor);
  return RDR_NO_ERROR;
}

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
struct token {
  const void* str;
  size_t len;
  bool new_line;
};

static enum sl_error
wstring_insert(struct sl_wstring* str, size_t id, const void* cstr)
{
  return sl_wstring_insert(str, id, (const wchar_t*)cstr);
}

static enum sl_error
wstring_insert_cstr(struct sl_wstring* str, size_t id, const void* cstr)
{
  return sl_wstring_insert_cstr(str, id, (const char*)cstr);
}

static enum sl_error
wstring_append(struct sl_wstring* str, const void* cstr)
{
  return sl_wstring_append(str, (const wchar_t*)cstr);
}

static enum sl_error
wstring_append_cstr(struct sl_wstring* str, const void* cstr)
{
  return sl_wstring_append_cstr(str, (const char*)cstr);
}

static void
tokenize(const void* str, struct token* tkn)
{
  wchar_t* ptr = NULL;
  assert(str && tkn);

  ptr = wcschr((const wchar_t*)str, L'\n');
  if(ptr) {
    *ptr = L'\0';
    tkn->new_line = true;
  } else {
    tkn->new_line = false;
  }
  tkn->str = str;
  tkn->len = wcslen((const wchar_t*)str);
}

static void
tokenize_cstr(const void* str, struct token* tkn)
{
  char* ptr = NULL;
  assert(str && tkn);

  ptr = strchr((const char*)str, '\n');
  if(ptr) {
    *ptr = '\0';
    tkn->new_line = true;
  } else {
    tkn->new_line = false;
  }
  tkn->str = str;
  tkn->len = strlen((const char*)str);
}

static size_t 
str_len(const void* str)
{
  return wcslen((const wchar_t*)str);
}

static size_t
str_len_cstr(const void* str)
{
  return strlen((const char*)str);
}

static FINLINE enum rdr_error
generic_print_string
  (struct rdr_term* term,
   enum rdr_term_output output,
   const void* str,
   const unsigned char color[3],
   size_t sizeof_char, 
   enum sl_error (*wstring_insert)(struct sl_wstring*, size_t, const void*),
   enum sl_error (*wstring_append)(struct sl_wstring*, const void*),
   void tokenize(const void*, struct token* tkn),
   size_t str_len(const void*))
{
  struct line* line = NULL;
  size_t len = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!term || !str || !color) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  if(0 == term->screen.lines_per_screen)
    goto exit;

  #define CALL(func) \
    do { \
      if(SL_NO_ERROR != (sl_err=func)) { \
        rdr_err = sl_to_rdr_error(sl_err); \
        goto error; \
      } \
    } while(0)
  len = str_len(str);
  if(output == RDR_TERM_CMDOUT) {
    line = term->screen.cmdbuf;
    CALL(wstring_insert(line->string, term->screen.cursor, str));
    CALL(sl_vector_insert_n
      (line->color_list, term->screen.cursor, len, color));
    term->screen.cursor += len;
  } else {
    void* end_str = NULL;
    void* ptr = NULL;

    /* Copy the submitted string into the mutable scratch buffer. */
    blob_clear(&term->scratch);
    rdr_err = blob_push_back(&term->scratch, str, (len + 1) * sizeof_char);
    assert(RDR_NO_ERROR == rdr_err);
    ptr = blob_buffer(&term->scratch);
    end_str = (void*)((uintptr_t)ptr + len * sizeof_char);
    while((uintptr_t)ptr < (uintptr_t)end_str) {
      struct token tkn;
      line = term->screen.outbuf;
      tokenize(ptr, &tkn);
      CALL(wstring_append(line->string, tkn.str));
      CALL(sl_vector_push_back_n(line->color_list, tkn.len, color));
      ptr = (void*)((uintptr_t)ptr + (tkn.len + 1) * sizeof_char);
      if(tkn.new_line)
        new_buf(&term->screen, RDR_TERM_STDOUT);
    }
  }
  #undef CALL
exit:
  return rdr_err;
error:
  goto exit;
}

/* Return the number of times str is wrapped. */
static size_t
wrap_count
  (const wchar_t* str,
   size_t len,
   size_t line_width,
   struct rdr_font* font)
{
  size_t i = 0;
  size_t wrap_count = 0;
  size_t width = 0;
  assert((str || !len) && font);

  wrap_count = 0;
  width = line_width;
  for(i = 0; i < len; ++i) {
    struct rdr_glyph glyph;
    assert(str[i] != L'\0');
    if(str[i] == L'\t') {
      RDR(get_font_glyph(font, L' ', &glyph));
      glyph.width *= TAB_WIDTH;
    } else if(str[i] == L'\n') {
      glyph.width = SIZE_MAX;
    } else {
      RDR(get_font_glyph(font, str[i], &glyph));
    }
    if(width >= glyph.width) {
      width -= glyph.width;
    } else {
      ++wrap_count;
      width = line_width;
    }
  }
  return wrap_count;
}

static void
setup_font(struct rdr_term* term)
{
  struct rdr_font_metrics metrics;
  size_t chars_per_line = 0;
  size_t lines_per_screen = 0;
  size_t max_nb_glyphs = 0;

  assert(term);

  /* Approximate the size of the glyph vertex buffer.  */
  RDR(get_font_metrics(term->font, &metrics));

  if(0 == metrics.min_glyph_width) {
    chars_per_line = 0;
  } else {
    chars_per_line =
      (term->width / metrics.min_glyph_width)
    + ((term->width % metrics.min_glyph_width) != 0);
  }
  if(0 == metrics.line_space) {
    lines_per_screen = 0;
  } else {
    lines_per_screen =
      (term->height / metrics.line_space)
    + ((term->height % metrics.line_space) != 0);
  }
  max_nb_glyphs = chars_per_line * lines_per_screen;
  printer_storage
    (term->sys,
     &term->printer,
     max_nb_glyphs,
     metrics.min_glyph_pos_y,
     &term->scratch);
  screen_storage(term->sys, &term->screen, lines_per_screen);
  new_buf(&term->screen, RDR_TERM_STDOUT);
  new_buf(&term->screen, RDR_TERM_CMDOUT);
}

static void
on_font_data_updated(struct rdr_font* font UNUSED, void* data)
{
  struct rdr_term* term = data;
  assert(font && term && term->font == font);
  setup_font(term);
}

static void
release_term(struct ref* ref)
{
  struct rdr_term* term = NULL;
  struct rdr_system* sys = NULL;
  assert(NULL != ref);

  term = CONTAINER_OF(ref, struct rdr_term, ref);

  if(term->font)
    RDR(font_ref_put(term->font));
  blob_release(&term->scratch);
  reset_screen(term->sys, &term->screen);
  shutdown_printer(term->sys, &term->printer);

  sys = term->sys;
  MEM_FREE(sys->allocator, term);
  RDR(system_ref_put(sys));
}

/*******************************************************************************
 *
 * Console functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rdr_error
rdr_create_term
  (struct rdr_system* sys,
   struct rdr_font* font,
   size_t width,
   size_t height,
   struct rdr_term** out_term)
{
  struct rdr_term* term = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!sys || !font || !out_term || !width || !height) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  term = MEM_CALLOC(sys->allocator, 1, sizeof(struct rdr_term));
  if(!term) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  ref_init(&term->ref);
  RDR(system_ref_get(sys));
  term->sys = sys;
  blob_init(term->sys->allocator, &term->scratch);
  term->width = width;
  term->height = height;

  rdr_err = init_printer(term->sys, &term->printer);
  if(rdr_err != RDR_NO_ERROR)
    goto error;
  rdr_err = init_screen(term->sys, &term->screen);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  RDR(term_font(term, font));

exit:
  if(out_term)
    *out_term = term;
  return rdr_err;
error:
  if(term) {
    RDR(term_ref_put(term));
    term = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_term_ref_get(struct rdr_term* term)
{
  if(!term)
    return RDR_INVALID_ARGUMENT;
  ref_get(&term->ref);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_term_ref_put(struct rdr_term* term)
{
  if(!term)
    return RDR_INVALID_ARGUMENT;
  ref_put(&term->ref, release_term);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_term_font(struct rdr_term* term, struct rdr_font* font)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  struct rdr_font* old_font = NULL;

  if(!term || !font) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  if(font != term->font) {
    old_font = term->font;
    RDR(font_ref_get(font));
    term->font = font;

    rdr_err = rdr_font_attach_callback
      (font, RDR_FONT_SIGNAL_UPDATE_DATA, on_font_data_updated, term);
    if(rdr_err != RDR_NO_ERROR)
      goto error;
    setup_font(term);
  }

exit:
  if(old_font) {
    RDR(font_ref_put(old_font));
    RDR(font_detach_callback
      (font, RDR_FONT_SIGNAL_UPDATE_DATA, on_font_data_updated, term));
  }
  return rdr_err;
error:
  if(old_font && old_font != term->font) {
    RDR(font_ref_put(term->font));
    term->font = old_font;
    old_font = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_term_translate_cursor(struct rdr_term* term, int trans)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!term) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  if(trans <= 0) {
    const size_t x = (size_t)abs(trans);
    term->screen.cursor -= MIN(x, term->screen.cursor);
  } else {
    size_t len = 0;
    const size_t x = (size_t)trans;
    SL(wstring_length(term->screen.cmdbuf->string, &len));
    term->screen.cursor += MIN(x, len - term->screen.cursor);
  }
exit:
  return rdr_err;
error:
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_term_print_wstring
  (struct rdr_term* term,
   enum rdr_term_output output,
   const wchar_t* str,
   const unsigned char color[3])
{
  return generic_print_string
    (term, output, str, color, sizeof(wchar_t), 
     wstring_insert, wstring_append, tokenize, str_len);
}

EXPORT_SYM enum rdr_error
rdr_term_print_string
  (struct rdr_term* term,
   enum rdr_term_output output,
   const char* str,
   const unsigned char color[3])
{
  return generic_print_string
    (term, output, str, color, sizeof(char), 
     wstring_insert_cstr, wstring_append_cstr, tokenize_cstr, str_len_cstr);
}

EXPORT_SYM enum rdr_error
rdr_term_print_wchar
  (struct rdr_term* term,
   enum rdr_term_output output,
   wchar_t ch,
   const unsigned char color[3])
{
  struct line* line = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!term || !color) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  if(L'\0' == ch || 0 == term->screen.lines_per_screen)
    goto exit;

  if(output == RDR_TERM_CMDOUT) {
    line = term->screen.cmdbuf;
    sl_err = sl_wstring_insert_wchar(line->string, term->screen.cursor, ch);
    if(SL_NO_ERROR != sl_err) {
      rdr_err = sl_to_rdr_error(sl_err);
      goto error;
    }
    sl_err = sl_vector_insert(line->color_list, term->screen.cursor, color);
    if(SL_NO_ERROR != sl_err) {
      rdr_err = sl_to_rdr_error(sl_err);
      goto error;
    }
    ++term->screen.cursor;
  } else {
    if(L'\n' == ch) {
      new_buf(&term->screen, RDR_TERM_STDOUT);
    } else {
      line = term->screen.outbuf;

      sl_err = sl_wstring_append_wchar(line->string, ch);
      if(SL_NO_ERROR != sl_err) {
        rdr_err = sl_to_rdr_error(sl_err);
        goto error;
      }
      sl_err = sl_vector_push_back(line->color_list, color);
      if(SL_NO_ERROR != sl_err) {
        rdr_err = sl_to_rdr_error(sl_err);
        goto error;
      }
    }
  }

exit:
  return rdr_err;
error:
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_clear_term(struct rdr_term* term, enum rdr_term_output output)
{
  if(!term)
    return RDR_INVALID_ARGUMENT;
  if(RDR_TERM_CMDOUT == output) {
    SL(clear_wstring(term->screen.cmdbuf->string));
    SL(clear_vector(term->screen.cmdbuf->color_list));
    term->screen.cursor = 0;
  } else {
    struct list_node* node = NULL;
    struct list_node* tmp = NULL;
    LIST_FOR_EACH_SAFE(node, tmp, &term->screen.stdout_line_list) {
      list_move(node, &term->screen.free_line_list);
    }
    SL(clear_wstring(term->screen.outbuf->string));
    SL(clear_vector(term->screen.outbuf->color_list));
    term->screen.scroll_id = 0;
  }
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_term_write_backspace(struct rdr_term* term)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!term) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  if(NULL == term->screen.cmdbuf)
    goto exit;

  if(term->screen.cursor > 0) {
    struct line* line = term->screen.cmdbuf;
    --term->screen.cursor;
    SL(wstring_erase_wchar(line->string, term->screen.cursor));
    SL(vector_remove(line->color_list, term->screen.cursor));
  }

exit:
  return rdr_err;
error:
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_term_write_tab(struct rdr_term* term)
{
  return rdr_term_print_wchar
    (term, RDR_TERM_CMDOUT, L'\t', RDR_TERM_COLOR_BLACK);
}

EXPORT_SYM enum rdr_error
rdr_term_write_return(struct rdr_term* term)
{
  if(!term)
    return RDR_INVALID_ARGUMENT;
  new_buf(&term->screen, RDR_TERM_CMDOUT);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_term_write_suppr(struct rdr_term* term)
{
  size_t len = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  if(!term) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  if(NULL != term->screen.cmdbuf) {
    SL(wstring_length(term->screen.cmdbuf->string, &len));
    if(term->screen.cursor != len) {
      ++term->screen.cursor;
      rdr_err = rdr_term_write_backspace(term);
    }
  }
exit:
  return rdr_err;
error:
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_term_dump(struct rdr_term* term, size_t* out_len, wchar_t* buffer)
{
  if(!term)
    return RDR_INVALID_ARGUMENT;

  if(term->screen.lines_per_screen && (out_len || buffer)) {
    struct list_node* node = NULL;
    wchar_t* b = buffer;
    size_t len = 0;

    /* temporary add the cmdbuf to the stdout_line_list. */
    assert(term->screen.cmdbuf);
    list_add(&term->screen.stdout_line_list, &term->screen.cmdbuf->node);

    LIST_FOR_EACH_REVERSE(node, &term->screen.stdout_line_list) {
      const struct line* line = CONTAINER_OF(node, struct line, node);
      size_t line_len = 0;
      SL(wstring_length(line->string, &line_len));

      len += line_len + 1; /* +1 <=> '\n' or '\0' char. */
      if(buffer) {
        const wchar_t* cstr = NULL;
        SL(wstring_get(line->string, &cstr));
        memcpy(b, cstr, line_len * sizeof(wchar_t));
        b[line_len] = L'\n';
        b += line_len + 1;
      }
    }
    len -= (len > 0); /* Remove the NULL char. */
    if(out_len)
      *out_len = len; 
    if(buffer)
      buffer[len] = L'\0';

    list_del(&term->screen.cmdbuf->node);

  }
  return RDR_NO_ERROR;
}

/*******************************************************************************
 *
 * Private functions.
 *
 ******************************************************************************/
enum rdr_error
rdr_draw_term(struct rdr_term* term)
{
  struct rdr_font_metrics metrics;
  struct rdr_glyph glyph;
  struct {
    size_t width;
    size_t x;
    size_t y;
  } cursor = { 0, 0, 0 };
  float vertex_data[SIZEOF_GLYPH_VERTEX / sizeof(float)];
  struct list_node* node = NULL;
  size_t nb_glyphs = 0;
  size_t x = 0;
  size_t y = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  memset(&vertex_data, 0, sizeof(vertex_data));

  if(!term) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  RDR(get_font_metrics(term->font, &metrics));
  if(!metrics.line_space)
    goto exit;

  /* Fill the scratch buffer with the glyph vertices. */
  blob_clear(&term->scratch);
  nb_glyphs = 0;
  y = metrics.line_space / 2;

  /* temporary add the cmdbuf to the stdout_line_list. */
  assert(term->screen.cmdbuf);
  list_add(&term->screen.stdout_line_list, &term->screen.cmdbuf->node);

  LIST_FOR_EACH(node, &term->screen.stdout_line_list) {
    const unsigned char (*col_lst)[3] = NULL;
    struct line* line = NULL;
    const wchar_t* cstr = NULL;
    size_t id = 0;
    size_t line_len = 0;
    size_t width = 0;
    size_t wrap = 0;
    size_t nb_colors = 0;

    #define SET_VERTEX_POS(data, x, y, z) data[0]=(x), data[1]=(y), data[2]=(z)
    #define SET_VERTEX_TEX(data, u, v)    data[3]=(u), data[4]=(v)
    #define SET_VERTEX_COL(data, r, g, b) \
      data[5]=(float)(r) * (1.f / 255.f), \
      data[6]=(float)(g) * (1.f / 255.f), \
      data[7]=(float)(b) * (1.f / 255.f)

    /* Do not draw the line that are cut by the terminal borders. */
    if(y + metrics.line_space > term->height)
      break;

    line = CONTAINER_OF(node, struct line, node);
    SL(wstring_length(line->string, &line_len));
    SL(wstring_get(line->string, &cstr));
    SL(vector_buffer(line->color_list, &nb_colors, NULL, NULL, (void**)&col_lst));
    assert(nb_colors == line_len);

    /* TODO write a wrap func which tokenizes the string with respect to the wrap
     * conditions. */
    wrap = wrap_count(cstr, line_len, term->width, term->font);
    width = term->width;
    id = 0;
    x = 0;
    y += wrap * metrics.line_space;

    /* Fill the scratch buffer with the glyphs of the line. */
    for(id = 0; cstr[id] != L'\0'; ++id) {
      struct { float x; float y; } translated_pos[2];
      size_t adjusted_width = 0;
      int cond = 0;

     do {
        assert(cstr[id] != L'\0');
        /* Handle special chars. */
        if(cstr[id] == L'\t') {
          RDR(get_font_glyph(term->font, L' ', &glyph));
          adjusted_width = glyph.width * TAB_WIDTH;
        } else if (cstr[id] == L'\n') {
          adjusted_width = SIZE_MAX;
        } else {
          RDR(get_font_glyph(term->font, cstr[id], &glyph));
          adjusted_width = glyph.width;
        }
        /* Wrap the line. */
        if(width >= adjusted_width) {
          width -= adjusted_width;
        } else {
          width = term->width;
          x = 0;
          y -= metrics.line_space;
        }
        if(term->screen.cmdbuf == line && id == term->screen.cursor) {
          cursor.width = MAX(glyph.width, glyph.pos[1].x - glyph.pos[0].x);
          cursor.x = x + glyph.pos[0].x;
          cursor.y = y;
        }
        cond = (y + metrics.line_space) > term->height;
        id += cond;
      } while(cond);

      SET_VERTEX_COL(vertex_data, col_lst[id][0],col_lst[id][1],col_lst[id][2]);

      translated_pos[0].x = glyph.pos[0].x + (float)x;
      translated_pos[0].y = glyph.pos[0].y + (float)y;
      translated_pos[1].x = glyph.pos[1].x + (float)x;
      translated_pos[1].y = glyph.pos[1].y + (float)y;

      /* bottom left. */
      SET_VERTEX_POS(vertex_data, translated_pos[0].x, translated_pos[1].y,0.f);
      SET_VERTEX_TEX(vertex_data, glyph.tex[0].x, glyph.tex[1].y);
      blob_push_back(&term->scratch, vertex_data, sizeof(vertex_data));
      /* top left. */
      SET_VERTEX_POS(vertex_data, translated_pos[0].x, translated_pos[0].y,0.f);
      SET_VERTEX_TEX(vertex_data, glyph.tex[0].x, glyph.tex[0].y);
      blob_push_back(&term->scratch, vertex_data, sizeof(vertex_data));
      /* top right. */
      SET_VERTEX_POS(vertex_data, translated_pos[1].x, translated_pos[0].y,0.f);
      SET_VERTEX_TEX(vertex_data, glyph.tex[1].x, glyph.tex[0].y);
      blob_push_back(&term->scratch, vertex_data, sizeof(vertex_data));
      /* bottom right. */
      SET_VERTEX_POS(vertex_data, translated_pos[1].x, translated_pos[1].y,0.f);
      SET_VERTEX_TEX(vertex_data, glyph.tex[1].x, glyph.tex[1].y);
      blob_push_back(&term->scratch, vertex_data, sizeof(vertex_data));

      ++nb_glyphs;
      x += adjusted_width;
    }
    if(term->screen.cmdbuf == line && id == term->screen.cursor) {
      RDR(get_font_glyph(term->font, L' ', &glyph));
      cursor.width = glyph.width;
      cursor.x = x;
      cursor.y = y;
    }
    y += (wrap + 1) * metrics.line_space;

    #undef SET_VERTEX_POS
    #undef SET_VERTEX_TEX
    #undef SET_VERTEX_COL
  }
  /* Remove the cmdbuf from the stdout. */
  list_del(&term->screen.cmdbuf->node);

  /* Send the glyph data to the printer and draw. */
  printer_data
    (term->sys, &term->printer, nb_glyphs, blob_buffer(&term->scratch));
  printer_draw
    (term->sys, term->font, &term->printer, term->width, term->height,
     cursor.x, cursor.y, cursor.width);

 exit:
  return rdr_err;
error:
  goto exit;
}

#undef TAB_WIDTH
#undef GLYPH_CACHE_TEX_UNIT
#undef VERTICES_PER_GLYPH
#undef INDICES_PER_GLYPH
#undef SIZEOF_GLYPH_VERTEX
#undef POS_ATTRIB_ID
#undef TEX_ATTRIB_ID
#undef COL_ATTRIB_ID
#undef NB_ATTRIBS

