#include "resources/regular/rsrc_context_c.h"
#include "resources/regular/rsrc_font_c.h"
#include "resources/rsrc_font.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdbool.h>

#ifndef NDEBUG
  #include <assert.h>
  #define FT(func) assert(0 == FT_##func)
#else
  #define FT(func) FT_##func
#endif

struct rsrc_font_library
{
  FT_Library handle;
};

struct rsrc_font
{
  struct rsrc_context* ctxt;
  FT_Face face;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static enum rsrc_error
ft_to_rsrc_error(FT_Error ft_err)
{
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;

  switch(ft_err) {
    case FT_Err_Ok:
      rsrc_err = RSRC_NO_ERROR;
      break;
    default:
      rsrc_err = RSRC_INTERNAL_ERROR;
      break;
  }
  return rsrc_err;
}

static size_t
sizeof_ft_pixel_mode(FT_Pixel_Mode mode)
{
  size_t size = 0;

  switch(mode) {
    case FT_PIXEL_MODE_NONE:
      size = 0;
      break;
    case FT_PIXEL_MODE_MONO:
    case FT_PIXEL_MODE_GRAY:
    case FT_PIXEL_MODE_GRAY2:
    case FT_PIXEL_MODE_GRAY4:
      size = 1;
      break;
    case FT_PIXEL_MODE_LCD:
    case FT_PIXEL_MODE_LCD_V:
      size = 3;
      break;
    default:
      assert(false);
      break;
  }
  return size;
}

static void
copy_bitmap_pixel
  (const FT_Bitmap* bitmap,
   size_t x,
   size_t y,
   unsigned char* pixel)
{
  const unsigned char* bmp_row = bitmap->buffer + y * bitmap->pitch;
  const unsigned char* bmp_byte = NULL;
  size_t bit_shift = 0;
  const char mode = bitmap->pixel_mode;

  if(mode == FT_PIXEL_MODE_MONO) {
    bmp_byte = bmp_row + x / 8;
    bit_shift = 7 - x % 8;
    *pixel = (*bmp_byte >> bit_shift) & 0x01;
  } else if(mode == FT_PIXEL_MODE_GRAY) {
    bmp_byte = bmp_row + x;
    *pixel = *bmp_byte;
  } else if(mode == FT_PIXEL_MODE_GRAY2) {
    bmp_byte = bmp_row + x / 4;
    bit_shift = (3 - x % 4) * 2;
    *pixel = (*bmp_byte >> bit_shift) & 0x03;
  } else if(mode == FT_PIXEL_MODE_GRAY4) {
    bmp_byte = bmp_row + x / 2;
    bit_shift = (1 - x % 2) * 4;
    *pixel = (*bmp_byte >> bit_shift) & 0x0F;
  } else if(mode == FT_PIXEL_MODE_LCD || mode == FT_PIXEL_MODE_LCD_V) {
    bmp_byte = bmp_row + x * 3;
    pixel[0] = bmp_byte[0];
    pixel[1] = bmp_byte[1];
    pixel[2] = bmp_byte[2];
  } else {
    /* Unexpected pixel mode. */
    assert(false);
  }
}

/*******************************************************************************
 *
 * Public functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rsrc_error
rsrc_create_font
  (struct rsrc_context* ctxt,
   const char* path,
   struct rsrc_font** out_font)
{
  struct rsrc_font* font = NULL;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;

  if(!ctxt || !out_font) {
    rsrc_err = RSRC_INVALID_ARGUMENT;
    goto error;
  }
  font = MEM_CALLOC(ctxt->allocator, 1, sizeof(struct rsrc_font));
  if(!font) {
    rsrc_err = RSRC_MEMORY_ERROR;
    goto error;
  }
  font->ctxt = ctxt;
  if(path) {
    rsrc_err = rsrc_load_font(font, path);
    if(rsrc_err != RSRC_NO_ERROR)
      goto error;
  }

exit:
  if(out_font)
    *out_font = font;
  return rsrc_err;

error:
  if(font) {
    MEM_FREE(ctxt->allocator, font);
    font = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_free_font(struct rsrc_font* font)
{
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;

  if(!font) {
    rsrc_err =  RSRC_INVALID_ARGUMENT;
    goto error;
  }
  FT(Done_Face(font->face));
  MEM_FREE(font->ctxt->allocator, font);

exit:
  return rsrc_err;
error:
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_load_font(struct rsrc_font* font, const char* path)
{
  FT_Error ft_err = 0;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;

  if(!font || !path) {
    rsrc_err = RSRC_INVALID_ARGUMENT;
    goto error;
  }
  ft_err = FT_New_Face(font->ctxt->font_lib->handle, path, 0, &font->face);
  if(0 != ft_err) {
    rsrc_err = ft_to_rsrc_error(ft_err);
    goto error;
  }
  /* Set a default char size of 16pt for a resolution of 96x96dpi. */
  FT(Set_Char_Size(font->face, 0, 16*64, 0, 96));

exit:
  return rsrc_err;
error:
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_font_bitmap
  (const struct rsrc_font* font,
   unsigned long int ch,
   size_t* width,
   size_t* height,
   size_t* bytes_per_pixel,
   unsigned char* buffer)
{
  const FT_Bitmap* bmp = NULL;
  size_t Bpp = 0;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;

  if(!font) {
    rsrc_err = RSRC_INVALID_ARGUMENT;
    goto error;
  }
  FT(Load_Char(font->face, (FT_ULong)ch, FT_LOAD_RENDER));
  bmp = &font->face->glyph->bitmap;
  Bpp = sizeof_ft_pixel_mode(bmp->pixel_mode);

  if(width)
    *width = bmp->width;
  if(height)
    *height = bmp->rows;
  if(bytes_per_pixel)
    *bytes_per_pixel = Bpp;
  if(buffer) {
    const size_t pitch = Bpp * bmp->width;
    int x, y;
    for(y = 0; y < bmp->rows; ++y) {
      unsigned char* row = buffer + y * pitch;
      for(x = 0; x < bmp->width; ++x) {
        unsigned char* pixel = row + x * Bpp;
        copy_bitmap_pixel(bmp, x, y, pixel);
      }
    }
  }
exit:
  return rsrc_err;
error:
  goto exit;
}

/*******************************************************************************
 *
 * Private functions.
 *
 ******************************************************************************/
enum rsrc_error
rsrc_init_font_library(struct rsrc_context* ctxt)
{
  struct rsrc_font_library* lib = NULL;
  FT_Error ft_err = 0;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;

  if(!ctxt) {
    rsrc_err = RSRC_INVALID_ARGUMENT;
    goto error;
  }
  lib = MEM_CALLOC(ctxt->allocator, 1, sizeof(struct rsrc_font_library));
  if(!lib) {
    rsrc_err = RSRC_MEMORY_ERROR;
    goto error;
  }
  ft_err = FT_Init_FreeType(&lib->handle);
  if(0 != ft_err) {
    rsrc_err = ft_to_rsrc_error(ft_err);
    goto error;
  }

exit:
  if(ctxt)
    ctxt->font_lib = lib;
  return rsrc_err;
error:
  if(lib) {
    if(lib->handle)
      FT_Done_FreeType(lib->handle);
    MEM_FREE(ctxt->allocator, lib);
    lib = NULL;
  }
  goto exit;
}

enum rsrc_error
rsrc_shutdown_font_library(struct rsrc_context* ctxt)
{
  FT_Error ft_err = 0;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;

  if(!ctxt) {
    rsrc_err = RSRC_INVALID_ARGUMENT;
    goto error;
  }
  ft_err = FT_Done_FreeType(ctxt->font_lib->handle);
  if(0 != ft_err) {
    rsrc_err = ft_to_rsrc_error(ft_err);
    goto error;
  }
  MEM_FREE(ctxt->allocator, ctxt->font_lib);
  ctxt->font_lib = NULL;
exit:
  return rsrc_err;
error:
  goto exit;
}

