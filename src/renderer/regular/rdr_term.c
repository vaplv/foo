#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_font_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/regular/rdr_term_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_font.h"
#include "renderer/rdr_system.h"
#include "renderer/rdr_term.h"
#include "stdlib/sl.h"
#include "sys/list.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <float.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <wchar.h>

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
  wchar_t* char_list;
  size_t free_space; /* In Pixels. */
};

struct screen {
  struct list_node free_line_list;
  struct list_node used_line_list;
  struct line* line_list;
  wchar_t* char_buffer; /* Buffer of line chars. */
  size_t line_len; /* Number of char per lines. Include the NULL Char. */
  size_t lines_per_screen;
  size_t scroll_id;
  size_t cursor;
};

struct printer {
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
  struct rdr_font* font;
  size_t max_nb_glyphs; /* Maximum number glyphs that the printer can draw. */
  size_t nb_glyphs; /* Number of glyphs currently drawn by the printer. */
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
#define XSTR(x) #x
#define STR(x) XSTR(x)

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

#undef XSTR
#undef STR

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
static struct line*
active_line(struct screen* scr)
{
  struct line* line = NULL;
  struct list_node* node = NULL;

  node = list_head(&scr->used_line_list);
  line = CONTAINER_OF(node, struct line, node);
  return line;
}

static void
reset_screen(struct rdr_system* sys, struct screen* scr)
{
  assert(scr);
  if(scr->char_buffer) {
    MEM_FREE(sys->allocator, scr->char_buffer);
    scr->char_buffer = NULL;
  }
  if(scr->line_list) {
    MEM_FREE(sys->allocator, scr->line_list);
    scr->line_list = NULL;
  }
  memset(scr, 0, sizeof(struct screen));
  list_init(&scr->free_line_list);
  list_init(&scr->used_line_list);
}

static enum rdr_error
screen_storage
  (struct rdr_system* sys,
   struct screen* scr,
   size_t chars_per_line,
   size_t lines_per_screen)
{
  wchar_t* char_list;
  /* Arbitrary set the max number of saved lines to 4 times the number of
   * printed lines per screen. */
  const size_t total_nb_lines = lines_per_screen * 4;
  /* +1 <=> null char. */
  const size_t line_len = chars_per_line + (chars_per_line != 0);
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  assert(sys && scr);

  reset_screen(sys, scr);
  scr->line_len = line_len;
  scr->lines_per_screen = lines_per_screen;
  scr->scroll_id = 0;

  if(!chars_per_line || !lines_per_screen)
    goto exit;

  scr->line_list = MEM_ALLOC
    (sys->allocator, total_nb_lines * sizeof(struct line));
  if(!scr->line_list) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  scr->char_buffer = MEM_ALLOC
    (sys->allocator, total_nb_lines * line_len * sizeof(wchar_t));
  if(!scr->char_buffer) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  for(i = 0, char_list = scr->char_buffer;
      i < total_nb_lines;
      ++i, char_list += line_len) {
    struct line* line = scr->line_list + i;
    list_init(&line->node);
    line->char_list = char_list;
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
   struct blob* scratch)
{
  struct rb_buffer_desc buffer_desc;
  memset(&buffer_desc, 0, sizeof(struct rb_buffer_desc));
  assert(sys && printer);

  if(printer->glyph_vertex_buffer) {
    RBI(sys->rb, buffer_ref_put(printer->glyph_vertex_buffer));
    printer->glyph_vertex_buffer = NULL;
  }
  if(printer->glyph_index_buffer) {
    RBI(sys->rb, buffer_ref_put(printer->glyph_index_buffer));
    printer->glyph_index_buffer = NULL;
  }

  printer->max_nb_glyphs = nb_glyphs;
  if(0 == nb_glyphs) {
    const int attrib_id_list[NB_ATTRIBS] = {
      [0] = POS_ATTRIB_ID,
      [1] = TEX_ATTRIB_ID,
      [2] = COL_ATTRIB_ID
    };
    RBI(sys->rb, remove_vertex_attrib
      (printer->varray, NB_ATTRIBS, attrib_id_list));
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
    RBI(sys->rb, create_buffer
      (sys->ctxt, &buffer_desc, NULL, &printer->glyph_vertex_buffer));

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
    RBI(sys->rb, create_buffer
      (sys->ctxt,
       &buffer_desc,
       blob_buffer(scratch),
       &printer->glyph_index_buffer));

    /* Setup the vertex array. */
    RBI(sys->rb, vertex_attrib_array
      (printer->varray,
       printer->glyph_vertex_buffer,
       NB_ATTRIBS,
       printer->attrib_list));
    RBI(sys->rb, vertex_index_array
      (printer->varray, printer->glyph_index_buffer));
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
    (size <= (printer->max_nb_glyphs*VERTICES_PER_GLYPH*SIZEOF_GLYPH_VERTEX));

  RBI(sys->rb, buffer_data(printer->glyph_vertex_buffer, 0, size, data));
  printer->nb_glyphs = nb_glyphs;
}

static void
printer_draw
  (struct rdr_system* sys,
   struct printer* printer,
   size_t width,
   size_t height)
{
  const float scale[3] = { 2.f/(float)width, 2.f/(float)height, 1.f };
  const float bias[3] = { -1.f, -1.f, 0.f };
  struct rb_blend_desc blend_desc;
  struct rb_depth_stencil_desc depth_stencil_desc;
  struct rb_viewport_desc viewport_desc;
  struct rb_tex2d* glyph_cache = NULL;
  struct rb_context* ctxt = NULL;
  memset(&depth_stencil_desc, 0, sizeof(depth_stencil_desc));
  memset(&viewport_desc, 0, sizeof(viewport_desc));
  assert(sys && printer);
  assert(width < INT_MAX && height < INT_MAX);

  ctxt = sys->ctxt;
  RDR(get_font_texture(printer->font, &glyph_cache));

  blend_desc.enable = 1;
  blend_desc.src_blend_RGB = RB_BLEND_SRC_ALPHA;
  blend_desc.src_blend_Alpha = RB_BLEND_ZERO;
  blend_desc.dst_blend_RGB = RB_BLEND_ONE_MINUS_SRC_ALPHA;
  blend_desc.dst_blend_Alpha = RB_BLEND_ONE;
  blend_desc.blend_op_RGB = RB_BLEND_OP_ADD;
  blend_desc.blend_op_Alpha = RB_BLEND_OP_ADD;
  RBI(sys->rb, blend(ctxt, &blend_desc));

  depth_stencil_desc.enable_depth_test = 0;
  depth_stencil_desc.enable_depth_write = 0;
  depth_stencil_desc.enable_stencil_test = 0;
  depth_stencil_desc.front_face_op.write_mask = 0;
  depth_stencil_desc.back_face_op.write_mask = 0;
  RBI(sys->rb, depth_stencil(ctxt, &depth_stencil_desc));

  viewport_desc.x = 0;
  viewport_desc.y = 0;
  viewport_desc.width = (int)width;
  viewport_desc.height = (int)height;
  viewport_desc.min_depth = 0.f;
  viewport_desc.max_depth = 1.f;
  RBI(sys->rb, viewport(ctxt, &viewport_desc));

  RBI(sys->rb, bind_tex2d(ctxt, glyph_cache, GLYPH_CACHE_TEX_UNIT));
  RBI(sys->rb, bind_sampler(ctxt, printer->sampler, GLYPH_CACHE_TEX_UNIT));

  RBI(sys->rb, bind_program(ctxt, printer->shading_program));
  RBI(sys->rb, uniform_data
    (printer->sampler_uniform, 1, (void*)(int[]){GLYPH_CACHE_TEX_UNIT}));
  RBI(sys->rb, uniform_data(printer->scale_uniform, 1, (void*)scale));
  RBI(sys->rb, uniform_data(printer->bias_uniform, 1, (void*)bias));

  RBI(sys->rb, bind_vertex_array(ctxt, printer->varray));
  RBI(sys->rb, draw_indexed
    (ctxt, RB_TRIANGLE_LIST, printer->nb_glyphs * INDICES_PER_GLYPH));

  blend_desc.enable = 0;
  RBI(sys->rb, blend(ctxt, &blend_desc));
  RBI(sys->rb, bind_tex2d(ctxt, NULL, GLYPH_CACHE_TEX_UNIT));
  RBI(sys->rb, bind_sampler(ctxt, NULL, GLYPH_CACHE_TEX_UNIT));
  RBI(sys->rb, bind_program(ctxt, NULL));
  RBI(sys->rb, bind_vertex_array(ctxt, NULL));
}

static void
printer_font
  (struct rdr_system* sys UNUSED,
   struct printer* printer,
   struct rdr_font* font)
{
  assert(sys && printer && font);

  if(printer->font)
    RDR(font_ref_put(printer->font));
  RDR(font_ref_get(font));
  printer->font = font;
}

static void
shutdown_printer(struct rdr_system* sys, struct printer* printer)
{
  struct rb_context* ctxt = NULL;
  assert(sys && printer);

  ctxt = sys->ctxt;

  if(printer->glyph_vertex_buffer)
    RBI(sys->rb, buffer_ref_put(printer->glyph_vertex_buffer));
  if(printer->glyph_index_buffer)
    RBI(sys->rb, buffer_ref_put(printer->glyph_index_buffer));
  if(printer->varray)
    RBI(sys->rb, vertex_array_ref_put(printer->varray));

  if(printer->shading_program) {
    RBI(sys->rb, detach_shader
      (printer->shading_program, printer->vertex_shader));
    RBI(sys->rb, detach_shader
      (printer->shading_program, printer->fragment_shader));
    RBI(sys->rb, program_ref_put(printer->shading_program));
  }
  if(printer->vertex_shader)
    RBI(sys->rb, shader_ref_put(printer->vertex_shader));
  if(printer->fragment_shader)
    RBI(sys->rb, shader_ref_put(printer->fragment_shader));

  if(printer->sampler)
    RBI(sys->rb, sampler_ref_put(printer->sampler));

  if(printer->sampler_uniform)
    RBI(sys->rb, uniform_ref_put(printer->sampler_uniform));
  if(printer->scale_uniform)
    RBI(sys->rb, uniform_ref_put(printer->scale_uniform));
  if(printer->bias_uniform)
    RBI(sys->rb, uniform_ref_put(printer->bias_uniform));

  if(printer->font)
    RDR(font_ref_put(printer->font));
}

static enum rdr_error
init_printer(struct rdr_system* sys, struct printer* printer)
{
  struct rb_sampler_desc sampler_desc;
  struct rb_context* ctxt = NULL;
  assert(sys && printer);

  ctxt = sys->ctxt;

  /* Vertex array. */
  printer->attrib_list[0].index = POS_ATTRIB_ID; /* Position .*/
  printer->attrib_list[0].stride = SIZEOF_GLYPH_VERTEX;
  printer->attrib_list[0].offset = 0;
  printer->attrib_list[0].type = RB_FLOAT3;
  printer->attrib_list[1].index = TEX_ATTRIB_ID; /* Tex coord. */
  printer->attrib_list[1].stride = SIZEOF_GLYPH_VERTEX;
  printer->attrib_list[1].offset = 3 * sizeof(float);
  printer->attrib_list[1].type = RB_FLOAT2;
  printer->attrib_list[2].index = COL_ATTRIB_ID; /* Color. */
  printer->attrib_list[2].stride = SIZEOF_GLYPH_VERTEX;
  printer->attrib_list[2].offset = 5 * sizeof(float);
  printer->attrib_list[2].type = RB_FLOAT3;
  RBI(sys->rb, create_vertex_array(ctxt, &printer->varray));

  /* Sampler. */
  sampler_desc.filter = RB_MIN_POINT_MAG_POINT_MIP_POINT;
  sampler_desc.address_u = RB_ADDRESS_CLAMP;
  sampler_desc.address_v = RB_ADDRESS_CLAMP;
  sampler_desc.address_w = RB_ADDRESS_CLAMP;
  sampler_desc.lod_bias = 0;
  sampler_desc.min_lod = -FLT_MAX;
  sampler_desc.max_lod = FLT_MAX;
  sampler_desc.max_anisotropy = 1;
  RBI(sys->rb, create_sampler(ctxt, &sampler_desc, &printer->sampler));

  /* Shaders. */
  RBI(sys->rb, create_shader
    (ctxt,
     RB_VERTEX_SHADER,
     print_vs_source,
     strlen(print_vs_source),
     &printer->vertex_shader));
  RBI(sys->rb, create_shader
    (ctxt,
     RB_FRAGMENT_SHADER,
     print_fs_source,
     strlen(print_fs_source),
     &printer->fragment_shader));

  /* Shading program. */
  RBI(sys->rb, create_program(ctxt, &printer->shading_program));
  RBI(sys->rb, attach_shader
    (printer->shading_program, printer->vertex_shader));
  RBI(sys->rb, attach_shader
    (printer->shading_program, printer->fragment_shader));
  RBI(sys->rb, link_program(printer->shading_program));

  RBI(sys->rb, get_named_uniform
    (ctxt, printer->shading_program, "glyph_cache", &printer->sampler_uniform));
  RBI(sys->rb, get_named_uniform
    (ctxt, printer->shading_program, "scale", &printer->scale_uniform));
  RBI(sys->rb, get_named_uniform
    (ctxt, printer->shading_program, "bias", &printer->bias_uniform));

  return RDR_NO_ERROR;
}

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
new_line(struct rdr_term* term)
{
  struct line* line = NULL;
  struct list_node* node = NULL;

  if(!term->screen.line_list)
    return;

  if(is_list_empty(&term->screen.free_line_list)) {
    node = list_tail(&term->screen.used_line_list);
  } else {
    node = list_head(&term->screen.free_line_list);
  }
  list_move(node, &term->screen.used_line_list);
  line = CONTAINER_OF(node, struct line, node);
  line->char_list[0] = L'\0';
  line->free_space = term->width;
  term->screen.cursor = 0;
}

static void
release_term(struct ref* ref)
{
  struct rdr_term* term = NULL;
  struct rdr_system* sys = NULL;
  assert(NULL != ref);

  term = CONTAINER_OF(ref, struct rdr_term, ref);
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
  size_t chars_per_line = 0;
  size_t lines_per_screen = 0;
  size_t line_space = 0;
  size_t max_nb_glyphs = 0;
  size_t min_glyph_width = 0;

  if(!term || !font)
    return RDR_INVALID_ARGUMENT;

  printer_font(term->sys, &term->printer, font);

  /* Approximate the size of the glyph vertex buffer.  */
  RDR(get_font_line_space(term->printer.font, &line_space));
  RDR(get_min_font_glyph_width(term->printer.font, &min_glyph_width));

  if(0 == min_glyph_width) {
    chars_per_line = 0;
  } else {
    chars_per_line =
      (term->width / min_glyph_width) + ((term->width % min_glyph_width) != 0);
  }
  if(0 == line_space) {
    lines_per_screen = 0;
  } else {
    lines_per_screen =
      (term->height / line_space) + ((term->height % line_space) != 0);
  }
  max_nb_glyphs = chars_per_line * lines_per_screen;
  printer_storage(term->sys, &term->printer, max_nb_glyphs, &term->scratch);
  screen_storage(term->sys, &term->screen, chars_per_line, lines_per_screen);
  new_line(term);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_term_print(struct rdr_term* term, const wchar_t* str)
{
  wchar_t* end_str = NULL;
  wchar_t* tkn = NULL;
  wchar_t* ptr = NULL;
  size_t len = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!term || !str) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  if(0 != term->screen.lines_per_screen) {
    /* Copy the submitted string into the mutable scratch buffer. */
    len = wcslen(str);
    blob_clear(&term->scratch);
    rdr_err = blob_push_back(&term->scratch, str, (len + 1) * sizeof(wchar_t));
    assert(RDR_NO_ERROR == rdr_err);
    tkn = blob_buffer(&term->scratch);

    end_str = tkn + len;
    while(tkn < end_str) {
      bool new_line_delimiter = false;

      ptr = wcschr(tkn, L'\n');
      if(ptr) {
        new_line_delimiter = true;
        *ptr = L'\0';
      }
      len = wcslen(tkn);
      while(len) {
        struct line* line = active_line(&term->screen);
        wchar_t* dst = line->char_list + term->screen.cursor;
        size_t char_id = 0;

        for(char_id = 0; L'\0' != tkn[char_id]; ++char_id) {
          struct rdr_glyph glyph;
          RDR(get_font_glyph(term->printer.font, tkn[char_id], &glyph));
          if(glyph.width > line->free_space)
            break;
          line->free_space -= glyph.width;
        }

        /* The char_id should lie into the char_list buffer. */
        assert
          ( (uintptr_t)(dst + char_id)
          < (uintptr_t)(line->char_list + term->screen.line_len));

        /* We ensure that the dst and src memory are not overlapping. */
        assert
          (!IS_MEMORY_OVERLAPPED
           (tkn,
            char_id * sizeof(wchar_t),
            dst,
            char_id * sizeof(wchar_t)));

        memcpy(dst, tkn, char_id * sizeof(wchar_t));
        dst[char_id] = L'\0';
        term->screen.cursor += char_id;
        tkn += char_id;
        len -= char_id;

        if(len != 0)
          new_line(term);
      }
      if(new_line_delimiter)
        new_line(term);
      ++tkn;
    }
  }
exit:
  return rdr_err;
error:
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_term_dump(const struct rdr_term* term, size_t* out_len, wchar_t* buffer)
{
  if(!term)
    return RDR_INVALID_ARGUMENT;

  if(term->screen.lines_per_screen && (out_len || buffer)) {
    struct list_node* node = NULL;
    wchar_t* b = buffer;
    size_t len = 0;

    LIST_FOR_EACH_REVERSE(node, &term->screen.used_line_list) {
      const struct line* line = CONTAINER_OF(node, struct line, node);
      const size_t line_len = wcslen(line->char_list);

      len += line_len + 1; /* +1 <=> '\n' or '\0' char. */
      if(buffer) {
        memcpy(b, line->char_list, line_len * sizeof(wchar_t));
        b[line_len] = L'\n';
        b += line_len + 1;
      }
    }
    if(out_len)
      *out_len = len;
    if(buffer && len)
      buffer[len - 1] = L'\0';
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
  float vertex_data[SIZEOF_GLYPH_VERTEX / sizeof(float)];
  struct list_node* node = NULL;
  size_t line_space = 0;
  size_t nb_glyphs = 0;
  size_t x = 0;
  size_t y = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  memset(&vertex_data, 0, sizeof(vertex_data));

  #define SET_VERTEX_POS(data, x, y, z) data[0]=(x), data[1]=(y), data[2]=(z)
  #define SET_VERTEX_TEX(data, u, v)    data[3]=(u), data[4]=(v)
  #define SET_VERTEX_COL(data, r, g, b) data[5]=(r), data[6]=(g), data[7]=(b)

  if(!term) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  RDR(get_font_line_space(term->printer.font, &line_space));
  if(!line_space)
    goto exit;

  /* Currently, the glyph is always white. */
  SET_VERTEX_COL(vertex_data, 1.f, 1.f, 1.f);

  /* Fill the scratch buffer with the glyph vertices. */
  blob_clear(&term->scratch);
  nb_glyphs = 0;
  y = 0;
  LIST_FOR_EACH(node, &term->screen.used_line_list) {
    struct line* line = NULL;
    size_t char_id = 0;

    /* Do not draw the line that are cut by the terminal borders. */
    y += line_space;
    if(y > term->height)
      break;

    /* Fill the scratch buffer with the glyphs of the line. */
    line = CONTAINER_OF(node, struct line, node);
    x = 0;
    for(char_id = 0;
        assert(char_id < term->screen.line_len),
        line->char_list[char_id] != '\0';
        ++char_id) {
      struct rdr_glyph glyph;
      struct {
        float x;
        float y;
      } translated_pos[2];
      RDR(get_font_glyph(term->printer.font, line->char_list[char_id], &glyph));

      translated_pos[0].x = glyph.pos[0].x + x;
      translated_pos[0].y = glyph.pos[0].y + y;
      translated_pos[1].x = glyph.pos[1].x + x;
      translated_pos[1].y = glyph.pos[1].y + y;

      /* top left. */
      SET_VERTEX_POS(vertex_data, translated_pos[0].x, translated_pos[0].y,0.f);
      SET_VERTEX_TEX(vertex_data, glyph.tex[0].x, glyph.tex[0].y);
      blob_push_back(&term->scratch, vertex_data, sizeof(vertex_data));
      /* bottom left. */
      SET_VERTEX_POS(vertex_data, translated_pos[0].x, translated_pos[1].y,0.f);
      SET_VERTEX_TEX(vertex_data, glyph.tex[0].x, glyph.tex[1].y);
      blob_push_back(&term->scratch, vertex_data, sizeof(vertex_data));
      /* bottom right. */
      SET_VERTEX_POS(vertex_data, translated_pos[1].x, translated_pos[1].y,0.f);
      SET_VERTEX_TEX(vertex_data, glyph.tex[1].x, glyph.tex[1].y);
      blob_push_back(&term->scratch, vertex_data, sizeof(vertex_data));
      /* top right. */
      SET_VERTEX_POS(vertex_data, translated_pos[1].x, translated_pos[0].y,0.f);
      SET_VERTEX_TEX(vertex_data, glyph.tex[1].x, glyph.tex[0].y);
      blob_push_back(&term->scratch, vertex_data, sizeof(vertex_data));

      ++nb_glyphs;
      x += glyph.width;
    }
  }
  /* Send the glyph data to the printer and draw. */
  printer_data
    (term->sys, &term->printer, nb_glyphs, blob_buffer(&term->scratch));
  printer_draw
    (term->sys, &term->printer, term->width, term->height);

  #undef SET_VERTEX_POS
  #undef SET_VERTEX_TEX
  #undef SET_VERTEX_COL

exit:
  return rdr_err;
error:
  goto exit;
}

#undef GLYPH_CACHE_TEX_UNIT
#undef VERTICES_PER_GLYPH
#undef INDICES_PER_GLYPH
#undef SIZEOF_GLYPH_VERTEX
#undef POS_ATTRIB_ID
#undef TEX_ATTRIB_ID
#undef COL_ATTRIB_ID
#undef NB_ATTRIBS

