#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_font.h"
#include "renderer/rdr_system.h"
#include "renderer/rdr_term.h"
#include "stdlib/sl.h"
#include "stdlib/sl_linked_list.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <string.h>
#include <wchar.h>

struct line {
  struct sl_node node;
  wchar_t* char_list;
};

struct screen {
  struct sl_node free_line_list;
  struct sl_node used_line_list;
  struct line* line_list;
  wchar_t* char_buffer; /* Buffer of line chars. */
  wchar_t* scratch_buffer; /* Work buffer. */
  size_t scratch_len;
  size_t width;
  size_t height;
  size_t scroll_id;
  size_t cursor;
};

struct rdr_term {
  struct ref ref;
  struct rdr_system* sys;
  struct rdr_font* font;
  struct screen screen;
  struct rb_buffer* glyph_quad;
  struct rb_buffer* glyph_parameters;
  struct rb_shader* vertex_shader;
  struct rb_shader* fragment_shader;
  struct rb_program* program;
  struct rb_vertex_array* glyph_varray;
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
  " glyph_tex = tex;\n"
  " glyph_col = col;\n"
  " gl_Position = pos;\n"
  "}\n";

static const char* print_fs_source =
  "#version 330\n"
  "uniform sampler2D glyph_cache;\n"
  "smooth in vec2 glyph_tex;\n"
  "flat   in vec4 glyph_col;\n"
  "out vec4 color;\n"
  "void main()\n"
  "{\n"
  " color = texture(glyph_cache, glyph_tex) * glyph_col;\n"
  "}\n";

/*******************************************************************************
 *
 * Helper functionss
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
  struct sl_node* node = NULL;

  SL(list_head(&scr->used_line_list, &node));
  line = CONTAINER_OF(node, struct line, node);
  active_line.freebuf = line->char_list + scr->cursor;
  active_line.freelen = scr->width - scr->cursor;
  return active_line;
}

static void
new_line(struct screen* scr)
{
  struct line* line = NULL;
  struct sl_node* node = NULL;
  bool is_empty = false;

  SL(is_list_empty(&scr->free_line_list, &is_empty));
  if(is_empty) {
    SL(list_tail(&scr->used_line_list, &node));
  } else {
    SL(list_head(&scr->free_line_list, &node));
  }
  SL(list_move(node, &scr->used_line_list));
  line = CONTAINER_OF(node, struct line, node);
  line->char_list[0] = '\0';
  scr->cursor = 0;
}

static int
ensure_scratch_len
  (struct mem_allocator* allocator,
   struct screen* scr,
   size_t len)
{
  int err = 0;

  if(len > scr->scratch_len) {
    scr->scratch_buffer = MEM_REALLOC
      (allocator, scr->scratch_buffer, len * sizeof(wchar_t));
    if(!scr->scratch_buffer)
      goto error;
    scr->scratch_len = len;
  }
exit:
  return err;
error:
  if(scr->scratch_buffer) {
    MEM_FREE(allocator, scr->scratch_buffer);
  }
  scr->scratch_buffer = NULL;
  scr->scratch_len = 0;
  err = -1;
  goto exit;
}

static void
shutdown_screen(struct mem_allocator* allocator, struct screen* scr)
{
  assert(scr);
  if(scr->char_buffer)
    MEM_FREE(allocator, scr->char_buffer);
  if(scr->scratch_buffer) {
    MEM_FREE(allocator, scr->scratch_buffer);
    scr->scratch_len = 0;
  }
  if(scr->line_list)
    MEM_FREE(allocator, scr->line_list);
  memset(scr, 0, sizeof(struct screen));
}

static enum rdr_error
init_screen
  (struct mem_allocator* allocator,
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
    (allocator, total_nb_lines * sizeof(struct line));
  if(!scr->line_list) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  scr->char_buffer = MEM_ALLOC
    (allocator, total_nb_lines * line_width * sizeof(wchar_t));
  if(!scr->char_buffer) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  scr->scratch_buffer = MEM_ALLOC(allocator, BUFSIZ * sizeof(wchar_t));
  if(!scr->scratch_buffer) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  scr->scratch_len = BUFSIZ;

  SL(init_node(&scr->free_line_list));
  SL(init_node(&scr->used_line_list));
  for(i = 0, char_list = scr->char_buffer;
      i < total_nb_lines;
      ++i, char_list += line_width) {
    struct line* line = scr->line_list + i;
    SL(init_node(&line->node));
    line->char_list = char_list;
    SL(list_add(&scr->free_line_list, &line->node));
  }
  scr->width = width;
  scr->height = height;
  scr->scroll_id = 0;
  new_line(scr);

exit:
  return rdr_err;
error:
  shutdown_screen(allocator, scr);
  goto exit;
}

static void
release_term(struct ref* ref)
{
  struct rdr_term* term = NULL;
  struct rdr_system* sys = NULL;
  assert(NULL != ref);

  term = CONTAINER_OF(ref, struct rdr_term, ref);
  shutdown_screen(term->sys->allocator, &term->screen);
  if(term->font)
    RDR(font_ref_put(term->font));
  if(term->glyph_quad) {
    int err = 0;
    err = term->sys->rb.free_buffer(term->sys->ctxt, term->glyph_quad);
    assert(0 == err);
  }
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
  /* Interleaved coords (float3) and UV (float2). */
  float glyph_vertices[] = {
    0.f, 1.f, 0.f, 0.f, 1.f,
    0.f, 0.f, 0.f, 0.f, 0.f,
    1.f, 1.f, 0.f, 1.f, 1.f,
    1.f, 0.f, 0.f, 1.f, 0.f
  };
  struct rb_buffer_desc buffer_desc;
  struct rdr_term* term = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int rb_err = 0;
  memset(&buffer_desc, 0, sizeof(buffer_desc));

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

  rdr_err = init_screen(term->sys->allocator, &term->screen, width, height);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  buffer_desc.size = sizeof(glyph_vertices);
  buffer_desc.target = RB_BIND_VERTEX_BUFFER;
  buffer_desc.usage = RB_BUFFER_USAGE_IMMUTABLE;
  rb_err = sys->rb.create_buffer
    (sys->ctxt, &buffer_desc, glyph_vertices, &term->glyph_quad);
  assert(0 == rb_err);

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
  ensure_scratch_len(term->sys->allocator, &term->screen, len + 1);
  term->screen.scratch_buffer = wcscpy(term->screen.scratch_buffer, str);

  tkn = term->screen.scratch_buffer;
  end_str = term->screen.scratch_buffer + len;
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
    struct sl_node* node = NULL;
    wchar_t* b = buffer;
    size_t len = 0;

    SL_LIST_FOR_EACH_REVERSE(node, &term->screen.used_line_list) {
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

