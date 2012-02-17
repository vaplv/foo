#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_font.h"
#include "renderer/rdr_system.h"
#include "renderer/rdr_term.h"
#include "stdlib/sl.h"
#include "sys/list.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <stdbool.h>
#include <string.h>
#include <wchar.h>

struct line {
  struct list_node node;
  wchar_t* char_list;
};

struct screen {
  struct list_node free_line_list;
  struct list_node used_line_list;
  struct line* line_list;
  wchar_t* char_buffer; /* Buffer of line chars. */
  wchar_t* scratch; /* Work buffer. */
  size_t scratch_len;
  size_t width;
  size_t height;
  size_t scroll_id;
  size_t cursor;
};

struct printer {
  struct rb_buffer* glyph_buffer;
  struct rb_vertex_array* varray;
  struct rb_shader* vertex_shader;
  struct rb_shader* fragment_shader;
  struct rb_program* shading_program;
  size_t glyph_buflen;
};

struct rdr_term {
  struct ref ref;
  struct rdr_system* sys;
  struct rdr_font* font;
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
  "in vec3 pos;\n"
  "in vec2 tex;\n"
  "in vec4 col; \n"
  "smooth out vec2 glyph_tex;\n"
  "flat   out vec4 glyph_col;\n"
  "void main()\n"
  "{\n"
  "  glyph_tex = tex;\n"
  "  glyph_col = col;\n"
  "  gl_Position = vec4(pos, 1);\n"
  "}\n";

static const char* print_fs_source =
  "#version 330\n"
  "uniform sampler2D glyph_cache;\n"
  "smooth in vec2 glyph_tex;\n"
  "flat   in vec4 glyph_col;\n"
  "out vec4 color;\n"
  "void main()\n"
  "{\n"
  "  color = texture(glyph_cache, glyph_tex) * glyph_col;\n"
  "}\n";

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
struct active_line {
  wchar_t* freebuf;
  size_t freelen;
};

static struct active_line
active_line(struct screen* scr)
{
  struct active_line active_line;
  struct line* line = NULL;
  struct list_node* node = NULL;

  node = list_head(&scr->used_line_list);
  line = CONTAINER_OF(node, struct line, node);
  active_line.freebuf = line->char_list + scr->cursor;
  active_line.freelen = scr->width - scr->cursor;
  return active_line;
}

static void
new_line(struct screen* scr)
{
  struct line* line = NULL;
  struct list_node* node = NULL;

  if(is_list_empty(&scr->free_line_list)) {
    node = list_tail(&scr->used_line_list);
  } else {
    node = list_head(&scr->free_line_list);
  }
  list_move(node, &scr->used_line_list);
  line = CONTAINER_OF(node, struct line, node);
  line->char_list[0] = L'\0';
  scr->cursor = 0;
}

static int
ensure_scratch_len
  (struct rdr_system* sys,
   struct screen* scr,
   size_t len)
{
  int err = 0;

  if(len > scr->scratch_len) {
    scr->scratch = MEM_REALLOC
      (sys->allocator, scr->scratch, len * sizeof(wchar_t));
    if(!scr->scratch)
      goto error;
    scr->scratch_len = len;
  }
exit:
  return err;
error:
  if(scr->scratch) {
    MEM_FREE(sys->allocator, scr->scratch);
  }
  scr->scratch = NULL;
  scr->scratch_len = 0;
  err = -1;
  goto exit;
}

static void
shutdown_screen(struct rdr_system* sys, struct screen* scr)
{
  assert(scr);
  if(scr->char_buffer)
    MEM_FREE(sys->allocator, scr->char_buffer);
  if(scr->scratch) {
    MEM_FREE(sys->allocator, scr->scratch);
    scr->scratch_len = 0;
  }
  if(scr->line_list)
    MEM_FREE(sys->allocator, scr->line_list);
  memset(scr, 0, sizeof(struct screen));
}

static enum rdr_error
init_screen
  (struct rdr_system* sys,
   struct screen* scr,
   size_t width,
   size_t height)
{
  wchar_t* char_list = NULL;
 /* Arbitrary set the max number of saved lines to 4 times the height of the
  * terminal. */
  const size_t total_nb_lines = height * 4;
  const size_t line_width = width + 1; /* +1 <=> null char. */
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  assert(scr);

  if(!width || !height) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  scr->line_list = MEM_ALLOC
    (sys->allocator, total_nb_lines * sizeof(struct line));
  if(!scr->line_list) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  scr->char_buffer = MEM_ALLOC
    (sys->allocator, total_nb_lines * line_width * sizeof(wchar_t));
  if(!scr->char_buffer) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  scr->scratch = MEM_ALLOC(sys->allocator, BUFSIZ * sizeof(wchar_t));
  if(!scr->scratch) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  scr->scratch_len = BUFSIZ;

  init_node(&scr->free_line_list);
  init_node(&scr->used_line_list);
  for(i = 0, char_list = scr->char_buffer;
      i < total_nb_lines;
      ++i, char_list += line_width) {
    struct line* line = scr->line_list + i;
    init_node(&line->node);
    line->char_list = char_list;
    list_add(&scr->free_line_list, &line->node);
  }
  scr->width = width;
  scr->height = height;
  scr->scroll_id = 0;
  new_line(scr);

exit:
  return rdr_err;
error:
  shutdown_screen(sys, scr);
  goto exit;
}

static void
shutdown_printer(struct rdr_system* sys, struct printer* printer)
{
  struct rb_context* ctxt = NULL;
  assert(sys && printer);

  ctxt = sys->ctxt;

  if(printer->glyph_buffer)
    RBI(sys->rb, buffer_ref_put(printer->glyph_buffer));
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
}

static enum rdr_error
init_printer(struct rdr_system* sys, struct printer* printer)
{
  struct rb_context* ctxt = NULL;
  struct rb_buffer_desc buffer_desc;
  struct rb_buffer_attrib attrib_list[3];
  const size_t sizeof_vertex = 8 * sizeof(float);
  memset(&buffer_desc, 0, sizeof(struct rb_buffer_desc));
  assert(sys && printer);

  ctxt = sys->ctxt;

  /* Vertex buffer. */
  buffer_desc.size = 0;
  buffer_desc.target = RB_BIND_VERTEX_BUFFER;
  buffer_desc.usage = RB_BUFFER_USAGE_DYNAMIC;
  RBI(sys->rb, create_buffer(ctxt, &buffer_desc, NULL, &printer->glyph_buffer));

  /* Vertex array. */
  attrib_list[0].index = 0; /* Position .*/
  attrib_list[0].stride = sizeof_vertex;
  attrib_list[0].offset = 0;
  attrib_list[0].type = RB_FLOAT3;
  attrib_list[1].index = 1; /* Tex coord. */
  attrib_list[1].stride = sizeof_vertex;
  attrib_list[1].offset = 3 * sizeof(float);
  attrib_list[1].type = RB_FLOAT2;
  attrib_list[2].index = 2; /* Color. */
  attrib_list[2].stride = sizeof_vertex;
  attrib_list[2].offset = 5 * sizeof(float);
  attrib_list[2].type = RB_FLOAT3;
  RBI(sys->rb, create_vertex_array(ctxt, &printer->varray));
  RBI(sys->rb, vertex_attrib_array
    (printer->varray, printer->glyph_buffer, 3, attrib_list));

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

  return RDR_NO_ERROR;
}

static void
release_term(struct ref* ref)
{
  struct rdr_term* term = NULL;
  struct rdr_system* sys = NULL;
  assert(NULL != ref);

  term = CONTAINER_OF(ref, struct rdr_term, ref);
  shutdown_screen(term->sys, &term->screen);
  shutdown_printer(term->sys, &term->printer);
  if(term->font)
    RDR(font_ref_put(term->font));

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

  if(!sys || !font || !out_term) {
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
  RDR(font_ref_get(font));
  term->font = font;

  rdr_err = init_screen(term->sys, &term->screen, width, height);
  if(rdr_err != RDR_NO_ERROR)
    goto error;
  rdr_err = init_printer(term->sys, &term->printer);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

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
  if(!term || !font)
    return RDR_INVALID_ARGUMENT;
  RDR(font_ref_put(term->font));
  RDR(font_ref_get(font));
  term->font = font;
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

  /* Copy the submitted string into the mutable scratch buffer. */
  len = wcslen(str);
  ensure_scratch_len(term->sys, &term->screen, len + 1);
  term->screen.scratch = wcscpy(term->screen.scratch, str);

  tkn = term->screen.scratch;
  end_str = term->screen.scratch + len;
  while(tkn < end_str) {
    bool new_line_delimiter = false;

    ptr = wcschr(tkn, L'\n');
    if(ptr) {
      new_line_delimiter = true;
      *ptr = L'\0';
    }
    len = wcslen(tkn);
    while(len) {
      struct active_line line = active_line(&term->screen);
      const size_t nb_chars = line.freelen > len ? len : line.freelen;

      /* We ensure that the dst and src memory are not overlapping. */
      assert
        (!IS_MEMORY_OVERLAPPED
         (tkn,
          nb_chars * sizeof(wchar_t),
          line.freebuf,
          nb_chars * sizeof(wchar_t)));

      memcpy(line.freebuf, tkn, nb_chars * sizeof(wchar_t));
      line.freebuf[len] = L'\0';
      term->screen.cursor += nb_chars;
      tkn += nb_chars;
      len -= nb_chars;

      if(len != 0)
        new_line(&term->screen);
    }
    if(new_line_delimiter)
      new_line(&term->screen);
    ++tkn;
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

  if(out_len || buffer) {
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

