#include "renderer/rdr.h"
#include "renderer/rdr_font.h"
#include "renderer/rdr_frame.h"
#include "renderer/rdr_system.h"
#include "renderer/rdr_term.h"
#include "resources/rsrc.h"
#include "resources/rsrc_context.h"
#include "resources/rsrc_font.h"
#include "window_manager/wm.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_input.h"
#include "window_manager/wm_window.h"
#include "sys/mem_allocator.h"
#include <limits.h>

#define FIRST_CHAR 32
#define LAST_CHAR 126
#define NB_CHARS (LAST_CHAR + 1) - FIRST_CHAR

static int g_exit = 0;

static void
char_clbk(wchar_t ch, enum wm_state state, void* data)
{
  assert(data);
  if(state == WM_PRESS) {
    struct rdr_term* term = data;
    if(ch >= FIRST_CHAR && ch <= LAST_CHAR)
      RDR(term_write_char(term, ch));
  }
}

static void
key_clbk(enum wm_key key, enum wm_state state, void* data)
{
  assert(data);
  if(state == WM_PRESS) {
    struct rdr_term* term = data;
    if(key == WM_KEY_ESC) {
      g_exit = 1;
    } else if(key == WM_KEY_ENTER) {
      RDR(term_write_return(term));
    } else if(key == WM_KEY_BACKSPACE) {
      RDR(term_write_backspace(term));
    } else if(key == WM_KEY_TAB) {
      RDR(term_write_tab(term));
    } else if(key == WM_KEY_RIGHT) {
      RDR(term_translate_cursor(term, 1));
    } else if(key == WM_KEY_LEFT) {
      RDR(term_translate_cursor(term, -1));
    } else if(key == WM_KEY_END) {
      RDR(term_translate_cursor(term, INT_MAX));
    } else if(key == WM_KEY_HOME) {
      RDR(term_translate_cursor(term, -INT_MAX));
    } else if(key == WM_KEY_DEL) {
      RDR(term_write_suppr(term));
    }
  }
}

int
main(int argc, char** argv)
{
  /* Window manager data structures. */
  struct wm_device* dev = NULL;
  struct wm_window* win = NULL;
  const struct wm_window_desc win_desc = { 800, 600, 0 };
  /* Renderer data structure. */
  struct rdr_glyph_desc glyph_desc_list[NB_CHARS];
  struct rdr_font* font = NULL;
  struct rdr_frame* frame = NULL;
  struct rdr_system* sys = NULL;
  struct rdr_term* term = NULL;
  /* Resources data structure. */
  struct rsrc_context* ctxt = NULL;
  struct rsrc_font* rfont = NULL;
  /* Miscellaneous data. */
  const char* driver_name = NULL;
  const char* font_name = NULL;
  size_t line_space = 0;
  size_t i = 0;
  bool b = false;

  if(argc != 3) {
    printf("usage: %s RB_DRIVER FONT\n", argv[0]);
    return -1;
  }
  driver_name = argv[1];
  font_name = argv[2];

  /* Setup the render font. */
  RSRC(create_context(NULL, &ctxt));
  RSRC(create_font(ctxt, font_name, &rfont));
  if(RSRC(is_font_scalable(rfont, &b)), b)
    RSRC(font_size(rfont, 18, 18));

  for(i = 0; i < NB_CHARS; ++i) {
    struct rsrc_glyph* glyph = NULL;
    struct rsrc_glyph_desc glyph_desc;
    size_t width = 0;
    size_t height = 0;
    size_t Bpp = 0;
    size_t size = 0;

    RSRC(font_glyph(rfont, (wchar_t)(i + FIRST_CHAR), &glyph));

    /* Get glyph desc. */
    RSRC(glyph_desc(glyph, &glyph_desc));
    glyph_desc_list[i].width = glyph_desc.width;
    glyph_desc_list[i].character = glyph_desc.character;
    glyph_desc_list[i].bitmap_left = glyph_desc.bbox.x_min;
    glyph_desc_list[i].bitmap_top = glyph_desc.bbox.y_min;

    /* Get glyph bitmap. */
    RSRC(glyph_bitmap(glyph, true, &width, &height, &Bpp, NULL));
    glyph_desc_list[i].bitmap.width = width;
    glyph_desc_list[i].bitmap.height = height;
    glyph_desc_list[i].bitmap.bytes_per_pixel = Bpp;
    glyph_desc_list[i].bitmap.buffer = NULL;
    size = width * height * Bpp;
    if(0 != size) {
      unsigned char* buffer = MEM_CALLOC(&mem_default_allocator, 1, size);
      RSRC(glyph_bitmap(glyph, true, NULL, NULL, NULL, buffer));
      glyph_desc_list[i].bitmap.buffer = buffer;
    }

    RSRC(glyph_ref_put(glyph));
  }

  RSRC(font_line_space(rfont, &line_space));
  RSRC(font_ref_put(rfont));
  RSRC(context_ref_put(ctxt));

  WM(create_device(NULL, &dev));
  WM(create_window(dev, &win_desc, &win));

  RDR(create_system(driver_name, NULL, &sys));
  RDR(create_frame(sys, &frame));
  RDR(background_color(frame, (float[]){0.05f, 0.05f, 0.05f}));
  RDR(create_font(sys, &font));
  RDR(font_data(font, line_space, NB_CHARS, glyph_desc_list));
  for(i = 0; i < NB_CHARS; ++i) {
    MEM_FREE(&mem_default_allocator, glyph_desc_list[i].bitmap.buffer);
  }

  RDR(create_term(sys, font, win_desc.width, win_desc.height, &term));
  RDR(term_write_string(term, L"Hello"));
  RDR(term_write_string(term, L" world!\n"));
  RDR(term_write_string
    (term,
     L"\"My definition of fragile code is, suppose you want to add a feature; "
     L"good code, there is one place where you add this feature and it fits; "
     L"fragile code, you've got to touch ten places\" (Ken Thompson)\n"));
  RDR(term_write_string(term, L"$"));

  WM(attach_char_callback(dev, &char_clbk, term));
  WM(attach_key_callback(dev, &key_clbk, term));

  while(!g_exit) {
    WM(flush_events(dev));
    RDR(frame_draw_term(frame, term));
    RDR(flush_frame(frame));
    WM(swap(win));
  }

  RDR(font_ref_put(font));
  RDR(frame_ref_put(frame));
  RDR(system_ref_put(sys));
  RDR(term_ref_put(term));

  WM(window_ref_put(win));
  WM(device_ref_put(dev));

  assert(MEM_ALLOCATED_SIZE(&mem_default_allocator) == 0);
  return 0;
}

