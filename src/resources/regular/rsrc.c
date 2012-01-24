#include "resources/rsrc.h"
#include "sys/sys.h"
#include <stdio.h>
#include <string.h>

EXPORT_SYM enum rsrc_error
rsrc_write_ppm
  (struct rsrc_context* ctxt,
   const char* path,
   size_t width,
   size_t height,
   size_t Bpp,
   const unsigned char* buffer)
{
  char buf[BUFSIZ];
  FILE* fp = NULL;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;

  if(!ctxt || (width && height && Bpp && !buffer)) {
    rsrc_err = RSRC_INVALID_ARGUMENT;
    goto error;
  }
  fp = fopen(path, "w");
  if(NULL == fp) {
    rsrc_err = RSRC_IO_ERROR;
    goto error;
  }

  #define FWRITE(fp, string) \
    do { \
      UNUSED const size_t i = fwrite(string, sizeof(char), strlen(string), fp); \
      assert(i == strlen(string) * sizeof(char)); \
    } while(0)
  #define SNPRINTF(b, sz, ...) \
    do { \
      UNUSED const size_t i = snprintf(b, sz, __VA_ARGS__); \
      assert(i < BUFSIZ); \
    } while(0)

  SNPRINTF(buf, BUFSIZ, "%s\n%zu %zu\n%i\n", "P3\n", width, height, 255);
  FWRITE(fp, buf);

  if(Bpp) {
    const size_t pitch = width * Bpp;
    size_t x, y;
    for(y = 0; y < height; ++y) {
      const unsigned char* row = buffer + y * pitch;
      for(x = 0; x < width; ++x) {
        const unsigned char* pixel = row + x * Bpp;
        SNPRINTF
          (buf, BUFSIZ,
           "%u %u %u\n",
           pixel[0],
           Bpp > 1 ? pixel[1] : pixel[0],
           Bpp > 2 ? pixel[2] : pixel[0]);
        FWRITE(fp, buf);
      }
      FWRITE(fp, "\n");
    }
  }

  #undef SNPRINTF
  #undef FWRITE

exit:
  if(fp)
    fclose(fp);
  return rsrc_err;

error:
  goto exit;
}

