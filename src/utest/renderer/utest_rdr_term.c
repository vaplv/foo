#include "renderer/rdr_font.h"
#include "renderer/rdr_system.h"
#include "renderer/rdr_term.h"
#include "resources/rsrc_context.h"
#include "resources/rsrc_font.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"
#include <wchar.h>

#define BAD_ARG RDR_INVALID_ARGUMENT
#define OK RDR_NO_ERROR

int
main(int argc, char** argv)
{
  const wchar_t char_list[] = {
    L' ', L'a', L'b', L'c', L'd', L'e', L'f', L'g', L'h', L'i', L'j', L'k',
    L'l', L'm', L'n', L'o', L'p', L'q', L'r', L's', L't', L'u', L'v', L'w',
    L'x', L'y', L'z'
  };
  wchar_t buffer[BUFSIZ];
  const char* driver_name = NULL;
  const char* font_name = NULL;
  const size_t nb_chars = sizeof(char_list) / sizeof(wchar_t);
  size_t line_space = 0;
  size_t i = 0;
  int err = 0;
  bool is_font_scalable = false;

  /* Resources data structure. */
  struct rsrc_context* ctxt = NULL;
  struct rsrc_font* ft = NULL;
  /* Window manager data structures. */
  struct wm_device* device = NULL;
  struct wm_window* window = NULL;
  struct wm_window_desc win_desc = {
    .width = 800, .height = 600, .fullscreen = false
  };
  /* Renderer data structures. */
  struct rdr_glyph_desc glyph_desc_list[nb_chars];
  struct rdr_font* font = NULL;
  struct rdr_system* sys = NULL;
  struct rdr_term* term = NULL;

  if(argc != 3) {
    printf("usage: %s RB_DRIVER FONT\n", argv[0]);
    goto error;
  }
  driver_name = argv[1];
  font_name = argv[2];

  CHECK(wm_create_device(NULL, &device), WM_NO_ERROR);
  CHECK(wm_create_window(device, &win_desc, &window), WM_NO_ERROR);

  CHECK(rsrc_create_context(NULL, &ctxt), RSRC_NO_ERROR);
  CHECK(rsrc_create_font(ctxt, font_name, &ft), RSRC_NO_ERROR);
  CHECK(rsrc_is_font_scalable(ft, &is_font_scalable), RSRC_NO_ERROR);
  if(is_font_scalable)
    CHECK(rsrc_font_size(ft, 8, 8), RSRC_NO_ERROR);

  for(i = 0; i < nb_chars; ++i) {
    struct rsrc_glyph* glyph = NULL;
    struct rsrc_glyph_desc glyph_desc;
    size_t width = 0;
    size_t height = 0;
    size_t Bpp = 0;
    size_t size = 0;

    CHECK(rsrc_font_glyph(ft, char_list[i], &glyph), RSRC_NO_ERROR);

    /* Get glyph desc. */
    CHECK(rsrc_glyph_desc(glyph, &glyph_desc), RSRC_NO_ERROR);
    glyph_desc_list[i].width = glyph_desc.width;
    glyph_desc_list[i].character = glyph_desc.character;
    glyph_desc_list[i].bitmap_left = glyph_desc.bbox.x_min;
    glyph_desc_list[i].bitmap_top = glyph_desc.bbox.y_min;
    /* Get glyph bitmap. */
    CHECK(rsrc_glyph_bitmap(glyph, true, &width, &height, &Bpp, NULL),
          RSRC_NO_ERROR);
    glyph_desc_list[i].bitmap.width = width;
    glyph_desc_list[i].bitmap.height = height;
    glyph_desc_list[i].bitmap.bytes_per_pixel = Bpp;
    glyph_desc_list[i].bitmap.buffer = NULL;
    size = width * height * Bpp;
    if(0 != size) {
      unsigned char* buffer = MEM_CALLOC(&mem_default_allocator, 1, size);
      CHECK(rsrc_glyph_bitmap(glyph, true, NULL, NULL, NULL, buffer),
            RSRC_NO_ERROR);
      glyph_desc_list[i].bitmap.buffer = buffer;
    }
    CHECK(rsrc_glyph_ref_put(glyph), RSRC_NO_ERROR);
  }
  CHECK(rsrc_font_line_space(ft, &line_space), RSRC_NO_ERROR);
  CHECK(rsrc_font_ref_put(ft), RSRC_NO_ERROR);
  CHECK(rsrc_context_ref_put(ctxt), RSRC_NO_ERROR);

  CHECK(rdr_create_system(driver_name, NULL, &sys), OK);
  CHECK(rdr_create_font(sys, &font), OK);
  CHECK(rdr_font_data(font, line_space, nb_chars, glyph_desc_list), OK);
  for(i = 0; i < nb_chars; ++i) {
    MEM_FREE(&mem_default_allocator, glyph_desc_list[i].bitmap.buffer);
  }

  CHECK(rdr_create_term(NULL, NULL, 0, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 0, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 0, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 0, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 80, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 80, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 80, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 80, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 0, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 0, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 0, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 0, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 80, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 80, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 80, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 80, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 0, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 0, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 0, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 0, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 80, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 80, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 80, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 80, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 0, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 0, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 0, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 0, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 80, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 80, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 80, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 80, 25, &term), OK);

  CHECK(rdr_term_font(NULL, NULL), BAD_ARG);
  CHECK(rdr_term_font(term, NULL), BAD_ARG);
  CHECK(rdr_term_font(NULL, font), BAD_ARG);
  CHECK(rdr_term_font(term, font), OK);

  CHECK(rdr_term_print(NULL, NULL), BAD_ARG);
  CHECK(rdr_term_print(term, NULL), BAD_ARG);
  CHECK(rdr_term_print(NULL, L"pouet"), BAD_ARG);
  CHECK(rdr_term_print(term, L"pouet"), OK);

  i = 0;
  CHECK(rdr_term_dump(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_term_dump(term, NULL, NULL), OK);
  CHECK(rdr_term_dump(term, &i, NULL), OK);

  /* We can't ensure for non scalable font that the text is not wrapped.
   * Consequently we can't easily predict the dump result. */
  if(is_font_scalable) {
    CHECK(i, 6);
    CHECK(i <= BUFSIZ, true);
    CHECK(rdr_term_dump(term, NULL, buffer), OK);
    CHECK(wcscmp(buffer, L"pouet"), 0);

    CHECK(rdr_term_print(term, L"\n pouet"), OK);
    CHECK(rdr_term_dump(term, &i, NULL), OK);
    CHECK(i <= BUFSIZ, true);
    CHECK(rdr_term_dump(term, &i, buffer), OK);
    CHECK(i, 13);
    CHECK(wcscmp(buffer, L"pouet\n pouet"), 0);

    CHECK(rdr_term_print(term, L"\ntest\n"), OK);
    CHECK(rdr_term_dump(term, &i, NULL), OK);
    CHECK(i <= BUFSIZ, true);
    CHECK(rdr_term_dump(term, &i, buffer), OK);
    CHECK(i, 19);
    CHECK(wcscmp(buffer, L"pouet\n pouet\ntest\n"), 0);
  }

  CHECK(rdr_term_ref_get(NULL), BAD_ARG);
  CHECK(rdr_term_ref_get(term), OK);
  CHECK(rdr_term_ref_put(NULL), BAD_ARG);
  CHECK(rdr_term_ref_put(term), OK);
  CHECK(rdr_term_ref_put(term), OK);

  CHECK(rdr_font_ref_put(font), OK);
  CHECK(rdr_system_ref_put(sys), OK);
  CHECK(wm_window_ref_put(window), WM_NO_ERROR);
  CHECK(wm_device_ref_put(device), WM_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

exit:
  return err;
error:
  err = -1;
  goto exit;
}
