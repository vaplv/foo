#include "render_backend/ogl3/rb_ogl3_context.h"
#include "render_backend/ogl3/rb_ogl3_texture.h"
#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/rb.h"
#include "sys/ref_count.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <string.h>

struct rb_framebuffer {
  struct ref ref;
  struct rb_framebuffer_desc desc;
  struct rb_context* ctxt;
  GLuint name;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void 
attach_render_target
  (GLenum attachment, 
   const struct rb_render_target* render_target)
{
  assert(render_target);
  switch(render_target->type) {
    case RB_RENDER_TARGET_TEXTURE2D:
      OGL(FramebufferTexture2D
          (GL_FRAMEBUFFER, 
           attachment,
           GL_TEXTURE_2D,
           ((struct rb_tex2d*)render_target->ressource)->name,
           render_target->desc.tex2d.mip_level));
          break;
    default: assert(0); break;
  }
}

static void
release_framebuffer(struct ref* ref)
{
  struct rb_context* ctxt  = NULL;
  struct rb_framebuffer* buffer = NULL;
  assert(ref);

  buffer = CONTAINER_OF(ref, struct rb_framebuffer, ref);
  ctxt = buffer->ctxt;
  OGL(DeleteFramebuffers(1, &buffer->name));
  MEM_FREE(ctxt->allocator, buffer);
  RB(context_ref_put(ctxt));
}

/*******************************************************************************
 *
 * Framebuffer functions.
 *
 ******************************************************************************/
EXPORT_SYM int
rb_create_framebuffer
  (struct rb_context* ctxt,
    const struct rb_framebuffer_desc* desc,
    struct rb_framebuffer** out_buffer)
{
  struct rb_framebuffer* buffer = NULL;
  int err = 0;

  if(UNLIKELY(!ctxt || !desc || !out_buffer))
    goto error;

  /* Multisampled framebuffer are not supported yet! */
  if(desc->sample_count != 1)
    goto error;

  buffer = MEM_CALLOC(ctxt->allocator, 1, sizeof(struct rb_framebuffer));
  if(!buffer)
    goto error;
  ref_init(&buffer->ref);
  RB(context_ref_get(ctxt));
  buffer->ctxt = ctxt;
  OGL(GenFramebuffers(1, &buffer->name));
  memcpy(&buffer->desc, desc, sizeof(struct rb_framebuffer_desc));

exit:
  if(out_buffer)
    *out_buffer = buffer;
  return err;
error:
  if(buffer) {
    RB(framebuffer_ref_put(buffer));
    buffer = NULL;
  }
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_framebuffer_ref_get(struct rb_framebuffer* buffer)
{
  if(UNLIKELY(!buffer))
    return -1;
  ref_get(&buffer->ref);
  return 0;
}

EXPORT_SYM int
rb_framebuffer_ref_put(struct rb_framebuffer* buffer)
{
  if(UNLIKELY(!buffer))
    return -1;
  ref_put(&buffer->ref, release_framebuffer);
  return 0;
}

EXPORT_SYM int
rb_bind_framebuffer
  (struct rb_context* ctxt,
   struct rb_framebuffer* buffer)
{
  if(UNLIKELY(!ctxt || !buffer))
    return -1;
  ctxt->state_cache.framebuffer_binding = buffer ? buffer->name : 0;
  OGL(BindFramebuffer(GL_FRAMEBUFFER, ctxt->state_cache.framebuffer_binding));
  return 0;
}

EXPORT_SYM int
rb_framebuffer_render_targets
  (struct rb_framebuffer* buffer,
   unsigned int count,
   const struct rb_render_target* render_target_list[],
   const struct rb_render_target* depth_stencil)
{
  unsigned int i = 0;
  int err = 0;

  if(UNLIKELY(!buffer || (count && !render_target_list)))
    goto error;

  OGL(BindFramebuffer(GL_FRAMEBUFFER, buffer->name));

  for(i = 0; i < count; ++i)
    attach_render_target(GL_COLOR_ATTACHMENT0 + i, render_target_list[i]);

  if(depth_stencil != NULL)
    attach_render_target(GL_DEPTH_STENCIL_ATTACHMENT, depth_stencil);

  OGL(BindFramebuffer
    (GL_FRAMEBUFFER, buffer->ctxt->state_cache.framebuffer_binding));
exit:
  return err;
error:
  err = -1;
  goto exit;
}

