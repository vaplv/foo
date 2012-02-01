#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_font.h"
#include "renderer/rdr_system.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdbool.h>

struct rdr_font {
  struct rdr_system* sys;
  struct ref ref;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_font(struct ref* ref)
{
  struct rdr_font* font = NULL;
  struct rdr_system* sys = NULL;
  assert(NULL != ref);

  font = CONTAINER_OF(ref, struct rdr_font, ref);
  sys = font->sys;
  MEM_FREE(font->sys->allocator, font);
  RDR(system_ref_put(sys));
}

/*******************************************************************************
 *
 * Font functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rdr_error
rdr_create_font(struct rdr_system* sys, struct rdr_font** out_font)
{
  struct rdr_font* font = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!sys || !out_font) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  font = MEM_CALLOC(sys->allocator, 1, sizeof(struct rdr_font));
  if(!font) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  ref_init(&font->ref);
  font->sys = sys;
  RDR(system_ref_get(sys));

exit:
  if(out_font)
    *out_font = font;
  return rdr_err;
error:
  if(font) {
    MEM_FREE(font->sys->allocator, font);
    font = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_font_ref_get(struct rdr_font* font)
{
  if(!font)
    return RDR_INVALID_ARGUMENT;
  ref_get(&font->ref);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_font_ref_put(struct rdr_font* font)
{
  if(!font)
    return RDR_INVALID_ARGUMENT;
  ref_put(&font->ref, release_font);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_font_data
  (struct rdr_font* font,
   size_t line_space UNUSED,
   size_t nb_glyphs,
   const struct rdr_glyph_desc* glyph_list)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!font || (nb_glyphs && !glyph_list)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  /* TODO Implement this function. */
  assert(false);

exit:
  return rdr_err;
error:
  goto exit;
}
