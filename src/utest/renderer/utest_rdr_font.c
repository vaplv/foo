#include "renderer/rdr_font.h"
#include "renderer/rdr_system.h"
#include "resources/rsrc_context.h"
#include "resources/rsrc_font.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"
#include <stdbool.h>
#include <string.h>

#define MEM_ERROR RDR_MEMORY_ERROR
#define BAD_ARG RDR_INVALID_ARGUMENT
#define OK RDR_NO_ERROR

static bool
is_driver_null(const char* name)
{
  const char* null_driver_name = "librbnull.so";
  char* p = NULL;
  if(name == NULL)
    return false;
  p = strstr(name, null_driver_name);
  return (p != NULL)
    && (p == name ? true : *(p-1) == '/')
    && (*(p + strlen(null_driver_name)) == '\0');
}

int
main(int argc, char** argv)
{
  const char* driver_name = NULL;
  const char* font_name = NULL;
  FILE* file = NULL;
  const size_t nb_glyphs = 94;
  const size_t nb_duplicated_glyphs = 5;
  const size_t total_nb_glyphs = nb_glyphs + nb_duplicated_glyphs;
  size_t width, height, Bpp;
  const unsigned char* bmp_cache = NULL;
  size_t i = 0;
  int err = 0;
  bool null_driver = false; 

  /* Resources data. */
  unsigned char* glyph_bitmap_list[total_nb_glyphs];
  struct rsrc_context* ctxt = NULL;
  struct rsrc_font* ft = NULL;
  struct rsrc_glyph* glyph = NULL;
  /* Window manager data structures. */
  struct wm_device* device = NULL;
  struct wm_window* window = NULL;
  struct wm_window_desc win_desc = {
    .width = 800, .height = 600, .fullscreen = false
  };
  /* Renderer data. */
  struct rdr_glyph_desc glyph_desc_list[total_nb_glyphs];
  struct rdr_font* font = NULL;
  struct rdr_system* sys = NULL;

  if(argc != 3) {
    printf("usage: %s RB_DRIVER FONT\n", argv[0]);
    goto error;
  }
  driver_name = argv[1];
  font_name = argv[2];
  null_driver = is_driver_null(driver_name);

  file = fopen(driver_name, "r");
  if(!file) {
    fprintf(stderr, "Invalid driver %s\n", driver_name);
    fclose(file);
    goto error;
  }
  fclose(file);

  file = fopen(driver_name, "r");
  if(!file) {
    fprintf(stderr, "Invalid font name %s\n", font_name);
    fclose(file);
    goto error;
  }
  fclose(file);

  CHECK(wm_create_device(NULL, &device), WM_NO_ERROR);
  CHECK(wm_create_window(device, &win_desc, &window), WM_NO_ERROR);
  CHECK(rsrc_create_context(NULL, &ctxt), RSRC_NO_ERROR);
  CHECK(rsrc_create_font(ctxt, font_name, &ft), RSRC_NO_ERROR);
  CHECK(rsrc_font_size(ft, 32, 32), RSRC_NO_ERROR);
  for(i = 0; i < total_nb_glyphs; ++i) {
    size_t width, height, Bpp;
    wchar_t character = 33 + (i % nb_glyphs);

    CHECK(rsrc_font_glyph(ft, character, &glyph),RSRC_NO_ERROR);
    CHECK(rsrc_glyph_bitmap
      (glyph, true, &width, &height, &Bpp, NULL), RSRC_NO_ERROR);

    glyph_bitmap_list[i] = MEM_CALLOC(&mem_default_allocator, width*height, Bpp);
    NCHECK(glyph_bitmap_list[i], NULL);
    CHECK(rsrc_glyph_bitmap
      (glyph, true, &width, &height, &Bpp, glyph_bitmap_list[i]), RSRC_NO_ERROR);

    glyph_desc_list[i].bitmap.width = width;
    glyph_desc_list[i].bitmap.height = height;
    glyph_desc_list[i].bitmap.bytes_per_pixel = Bpp;
    glyph_desc_list[i].bitmap.buffer = glyph_bitmap_list[i];

    CHECK(rsrc_glyph_width(glyph, &width), RSRC_NO_ERROR);
    glyph_desc_list[i].width = width;
    glyph_desc_list[i].character = character;

    CHECK(rsrc_free_glyph(glyph), RSRC_NO_ERROR);
  }

  CHECK(rdr_create_system(driver_name, NULL, &sys), OK);

  CHECK(rdr_create_font(NULL, NULL), BAD_ARG);
  CHECK(rdr_create_font(sys, NULL), BAD_ARG);
  CHECK(rdr_create_font(NULL, &font), BAD_ARG);
  CHECK(rdr_create_font(sys, &font), OK);

  CHECK(rdr_font_bitmap_cache(NULL, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, &width, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, NULL, &height, NULL, NULL), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, &width, &height, NULL, NULL), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, NULL, NULL, &Bpp, NULL), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, &width, NULL, &Bpp, NULL), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, NULL, &height, &Bpp, NULL), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, &width, &height, &Bpp, NULL), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, NULL, NULL, NULL, &bmp_cache), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, &width, NULL,NULL, &bmp_cache), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, NULL, &height,NULL,&bmp_cache), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, &width, &height, NULL,&bmp_cache), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, NULL, NULL, &Bpp, &bmp_cache), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, &width, NULL, &Bpp, &bmp_cache), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, NULL, &height, &Bpp, &bmp_cache), BAD_ARG);
  CHECK(rdr_font_bitmap_cache(NULL, &width, &height, &Bpp,&bmp_cache), BAD_ARG);

  CHECK(rdr_font_bitmap_cache(font, NULL, NULL, NULL, NULL), OK);
  CHECK(rdr_font_bitmap_cache(font, &width, NULL, NULL, NULL), OK);
  CHECK(rdr_font_bitmap_cache(font, NULL, &height, NULL, NULL), OK);
  CHECK(rdr_font_bitmap_cache(font, &width, &height, NULL, NULL), OK);
  CHECK(rdr_font_bitmap_cache(font, NULL, NULL, &Bpp, NULL), OK);
  CHECK(rdr_font_bitmap_cache(font, &width, NULL, &Bpp, NULL), OK);
  CHECK(rdr_font_bitmap_cache(font, NULL, &height, &Bpp, NULL), OK);
  CHECK(rdr_font_bitmap_cache(font, &width, &height, &Bpp, NULL), OK);
  CHECK(rdr_font_bitmap_cache(font, NULL, NULL, NULL, &bmp_cache), OK);
  CHECK(rdr_font_bitmap_cache(font, &width, NULL, NULL, &bmp_cache), OK);
  CHECK(rdr_font_bitmap_cache(font, NULL, &height, NULL, &bmp_cache), OK);
  CHECK(rdr_font_bitmap_cache(font, &width, &height, NULL, &bmp_cache), OK);
  CHECK(rdr_font_bitmap_cache(font, NULL, NULL, &Bpp, &bmp_cache), OK);
  CHECK(rdr_font_bitmap_cache(font, &width, NULL, &Bpp, &bmp_cache), OK);
  CHECK(rdr_font_bitmap_cache(font, NULL, &height, &Bpp, &bmp_cache), OK);
  CHECK(rdr_font_bitmap_cache(font, &width, &height, &Bpp, &bmp_cache), OK);

  width = height = Bpp = 1;
  bmp_cache = (void*)0xdeadbeef;
  CHECK(rdr_font_bitmap_cache(font, &width, &height, &Bpp, &bmp_cache), OK);
  CHECK(bmp_cache, NULL);
  CHECK(width, 0);
  CHECK(height, 0);
  CHECK(Bpp, 0);
   
  CHECK(rdr_font_data(NULL, 0, 0, NULL), BAD_ARG);
  CHECK(rdr_font_data(font, 0, 0, NULL), OK);
  CHECK(rsrc_font_line_space(ft, &i), RSRC_NO_ERROR);
  if(null_driver) {
    CHECK(rdr_font_data(font, i, nb_glyphs, glyph_desc_list), MEM_ERROR);
  } else {
    CHECK(rdr_font_data(font, i, nb_glyphs, glyph_desc_list), OK);
    CHECK(rdr_font_bitmap_cache(font, &width, &height, &Bpp, &bmp_cache), OK);
    NCHECK(bmp_cache, NULL);
    NCHECK(width, 0);
    NCHECK(height, 0);
    NCHECK(Bpp, 0);

    CHECK(rsrc_write_ppm
      (ctxt, "/tmp/font_cache.ppm", width, height, Bpp, bmp_cache), 
      RSRC_NO_ERROR);
  }

  CHECK(rdr_font_ref_get(NULL), BAD_ARG);
  CHECK(rdr_font_ref_get(font), OK);
  CHECK(rdr_font_ref_put(NULL), BAD_ARG);
  CHECK(rdr_font_ref_put(font), OK);
  CHECK(rdr_font_ref_put(font), OK);

  CHECK(rdr_system_ref_put(sys), OK);

  for(i = 0; i < total_nb_glyphs; ++i)
    MEM_FREE(&mem_default_allocator, glyph_bitmap_list[i]);

  CHECK(rsrc_free_font(ft), RSRC_NO_ERROR);
  CHECK(rsrc_context_ref_put(ctxt), RSRC_NO_ERROR);
  CHECK(wm_free_window(device, window), WM_NO_ERROR);
  CHECK(wm_free_device(device), WM_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

exit:
  return err;

error:
  err = -1;
  goto exit;
}
