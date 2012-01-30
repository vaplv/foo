#include "resources/rsrc_context.h"
#include "resources/rsrc_font.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include <stdbool.h>
#include <stdio.h>

#define OK RSRC_NO_ERROR
#define BAD_ARG RSRC_INVALID_ARGUMENT

int
main(int argc, char** argv)
{
  char buf[BUFSIZ];
  struct rsrc_context* ctxt = NULL;
  struct rsrc_font* font = NULL;
  struct rsrc_glyph* glyph = NULL;
  const char* path = NULL;
  unsigned char* buffer = NULL;
  size_t buffer_size = 0;
  size_t h = 0;
  size_t w = 0;
  size_t Bpp = 0;
  size_t i = 0;
  bool b = false;

  if(argc != 2) {
    printf("usage: %s FONT\n", argv[0]);
    goto error;
  }
  path = argv[1];

  CHECK(rsrc_create_context(NULL, &ctxt), OK);

  CHECK(rsrc_create_font(NULL, NULL, NULL), BAD_ARG);
  CHECK(rsrc_create_font(ctxt, NULL, NULL), BAD_ARG);
  CHECK(rsrc_create_font(NULL, NULL, &font), BAD_ARG);
  CHECK(rsrc_create_font(ctxt, NULL, &font), OK);

  CHECK(rsrc_load_font(NULL, NULL), BAD_ARG);
  CHECK(rsrc_load_font(font, NULL), BAD_ARG);
  CHECK(rsrc_load_font(NULL, path), BAD_ARG);
  CHECK(rsrc_load_font(font, path), OK);

  CHECK(rsrc_font_size(NULL, 0, 0), BAD_ARG);
  CHECK(rsrc_font_size(font, 0, 0), BAD_ARG);
  CHECK(rsrc_font_size(NULL, 32, 0), BAD_ARG);
  CHECK(rsrc_font_size(font, 32, 0), BAD_ARG);
  CHECK(rsrc_font_size(NULL, 0, 64), BAD_ARG);
  CHECK(rsrc_font_size(font, 0, 64), BAD_ARG);
  CHECK(rsrc_font_size(NULL, 32, 64), BAD_ARG);
  CHECK(rsrc_font_size(font, 32, 64), OK);

  CHECK(rsrc_font_size(font, 16, 16), OK);

  CHECK(rsrc_font_line_space(NULL, NULL), BAD_ARG);
  CHECK(rsrc_font_line_space(font, NULL), BAD_ARG);
  CHECK(rsrc_font_line_space(NULL, &i), BAD_ARG);
  CHECK(rsrc_font_line_space(font, &i), OK);

  CHECK(rsrc_font_glyph(NULL, L'a', NULL), BAD_ARG);
  CHECK(rsrc_font_glyph(font, L'a', NULL), BAD_ARG);
  CHECK(rsrc_font_glyph(NULL, L'a', &glyph), BAD_ARG);
  CHECK(rsrc_font_glyph(font, L'a', &glyph), OK);

  CHECK(rsrc_glyph_width(NULL, NULL), BAD_ARG);
  CHECK(rsrc_glyph_width(glyph, NULL), BAD_ARG);
  CHECK(rsrc_glyph_width(NULL, &i), BAD_ARG);
  CHECK(rsrc_glyph_width(glyph, &i), OK);

  CHECK(rsrc_glyph_bitmap(NULL, true, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(rsrc_glyph_bitmap(glyph, true, NULL, NULL, NULL, NULL), OK);
  CHECK(rsrc_glyph_bitmap(glyph, true, &w, NULL, NULL, NULL), OK);
  CHECK(rsrc_glyph_bitmap(glyph, true, NULL, &h, NULL, NULL), OK);
  CHECK(rsrc_glyph_bitmap(glyph, true, &w, &h, NULL, NULL), OK);
  CHECK(rsrc_glyph_bitmap(glyph, true, NULL, NULL, &Bpp, NULL), OK);
  CHECK(rsrc_glyph_bitmap(glyph, true, &w, NULL, &Bpp, NULL), OK);
  CHECK(rsrc_glyph_bitmap(glyph, true, NULL, &h, &Bpp, NULL), OK);
  CHECK(rsrc_glyph_bitmap(glyph, true, &w, &h, &Bpp, NULL), OK);
  NCHECK(w, 0);
  NCHECK(h, 0);
  NCHECK(Bpp, 0);

  buffer = MEM_CALLOC(&mem_default_allocator, w*h*Bpp, sizeof(unsigned char));
  NCHECK(buffer, NULL);
  buffer_size = w * h * Bpp * sizeof(unsigned char);

  CHECK(rsrc_glyph_bitmap(glyph, false, NULL, NULL, NULL, buffer), OK);
  CHECK(rsrc_glyph_bitmap(glyph, false, &w, NULL, NULL, buffer), OK);
  CHECK(rsrc_glyph_bitmap(glyph, false, NULL, &h, NULL, buffer), OK);
  CHECK(rsrc_glyph_bitmap(glyph, false, &w, &h, NULL, buffer), OK);
  CHECK(rsrc_glyph_bitmap(glyph, false, NULL, NULL, &Bpp, buffer), OK);
  CHECK(rsrc_glyph_bitmap(glyph, false, &w, NULL, &Bpp, buffer), OK);
  CHECK(rsrc_glyph_bitmap(glyph, false, NULL, &h, &Bpp, buffer), OK);
  CHECK(rsrc_glyph_bitmap(glyph, false, &w, &h, &Bpp, buffer), OK);

  CHECK(rsrc_free_glyph(NULL), BAD_ARG);
  CHECK(rsrc_free_glyph(glyph), OK);

  b = true;
  for(i = 0; b && i < w*h*Bpp; ++i)
    b = ((int)buffer[i] == 0);
  CHECK(b, false);

  for(i = 32; i < 127; ++i) {
    size_t required_buffer_size = 0;
    CHECK(rsrc_font_glyph(font, (wchar_t)i, &glyph), OK);

    CHECK(rsrc_glyph_bitmap(glyph, true, &w, &h, &Bpp, NULL), OK);
    required_buffer_size = w * h * Bpp * sizeof(unsigned char);
    if(required_buffer_size > buffer_size) {
      buffer = MEM_REALLOC(&mem_default_allocator,buffer, required_buffer_size);
      NCHECK(buffer, NULL);
      buffer_size = required_buffer_size;
    }
    CHECK(rsrc_glyph_bitmap(glyph, true, &w, &h, &Bpp, buffer), OK);
    NCHECK(snprintf(buf, BUFSIZ, "/tmp/%.3zu.ppm", i - 32), BUFSIZ);
    CHECK(rsrc_write_ppm(ctxt, buf, w, h, Bpp, buffer), OK);
    CHECK(rsrc_free_glyph(glyph), OK);
  }
  MEM_FREE(&mem_default_allocator, buffer);

  CHECK(rsrc_free_font(NULL), BAD_ARG);
  CHECK(rsrc_free_font(font), OK);

  CHECK(rsrc_context_ref_put(ctxt), OK);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;

error:
  return -1;
}

