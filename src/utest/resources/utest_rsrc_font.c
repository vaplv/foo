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

  CHECK(rsrc_font_bitmap(NULL, 'a', NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(rsrc_font_bitmap(font, 'a', NULL, NULL, NULL, NULL), OK);
  CHECK(rsrc_font_bitmap(font, 'a', &w, NULL, NULL, NULL), OK);
  CHECK(rsrc_font_bitmap(font, 'a', NULL, &h, NULL, NULL), OK);
  CHECK(rsrc_font_bitmap(font, 'a', &w, &h, NULL, NULL), OK);
  CHECK(rsrc_font_bitmap(font, 'a', NULL, NULL, &Bpp, NULL), OK);
  CHECK(rsrc_font_bitmap(font, 'a', &w, NULL, &Bpp, NULL), OK);
  CHECK(rsrc_font_bitmap(font, 'a', NULL, &h, &Bpp, NULL), OK);
  CHECK(rsrc_font_bitmap(font, 'a', &w, &h, &Bpp, NULL), OK);
  NCHECK(w, 0);
  NCHECK(h, 0);
  NCHECK(Bpp, 0);

  buffer = MEM_CALLOC(&mem_default_allocator, w*h*Bpp, sizeof(unsigned char));
  NCHECK(buffer, NULL);
  buffer_size = w * h * Bpp * sizeof(unsigned char); 

  CHECK(rsrc_font_bitmap(font, 'a', NULL, NULL, NULL, buffer), OK);
  CHECK(rsrc_font_bitmap(font, 'a', &w, NULL, NULL, buffer), OK);
  CHECK(rsrc_font_bitmap(font, 'a', NULL, &h, NULL, buffer), OK);
  CHECK(rsrc_font_bitmap(font, 'a', &w, &h, NULL, buffer), OK);
  CHECK(rsrc_font_bitmap(font, 'a', NULL, NULL, &Bpp, buffer), OK);
  CHECK(rsrc_font_bitmap(font, 'a', &w, NULL, &Bpp, buffer), OK);
  CHECK(rsrc_font_bitmap(font, 'a', NULL, &h, &Bpp, buffer), OK);
  CHECK(rsrc_font_bitmap(font, 'a', &w, &h, &Bpp, buffer), OK);

  b = true;
  for(i = 0; b && i < w*h*Bpp; ++i)
    b = ((int)buffer[i] == 0);
  CHECK(b, false);

  for(i = 1; i < 128; ++i) {
    size_t required_buffer_size = 0;

    CHECK(rsrc_font_bitmap(font, (unsigned long int)i, &w, &h, &Bpp, NULL), OK);
    required_buffer_size = w * h * Bpp * sizeof(unsigned char);
    if(required_buffer_size > buffer_size) {
      buffer = MEM_REALLOC(&mem_default_allocator,buffer, required_buffer_size);
      NCHECK(buffer, NULL);
      buffer_size = required_buffer_size;
    }
    CHECK(rsrc_font_bitmap(font, (unsigned long int)i, &w, &h, &Bpp, buffer), OK);
    NCHECK(snprintf(buf, BUFSIZ, "/tmp/%c.ppm", (char)i), BUFSIZ);
    CHECK(rsrc_write_ppm(ctxt, buf, w, h, Bpp, buffer), OK);
  }
  MEM_FREE(&mem_default_allocator, buffer);

  CHECK(rsrc_free_font(NULL), BAD_ARG);
  CHECK(rsrc_free_font(font), OK);

  CHECK(rsrc_free_context(ctxt), OK);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;

error:
  return -1;
}

