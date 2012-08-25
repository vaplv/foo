#include "render_backend/ogl3/rb_ogl3_context.h"
#include "render_backend/ogl3/rb_ogl3_texture.h"
#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/rb.h"
#include "sys/ref_count.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

struct render_target {
  enum rb_render_target_type type;
  void* resource;
};

struct rb_framebuffer {
  struct ref ref;
  struct rb_framebuffer_desc desc;
  struct rb_context* ctxt;
  GLuint name;
  struct render_target depth_stencil;
  struct render_target render_target_list[];
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
/* Retrieve the render target of the buffer from the OGL attachment. */
static struct render_target*
attachment_to_render_target(struct rb_framebuffer* buffer, int attachment)
{
  struct render_target* rt = NULL;
  assert(buffer);
  if(attachment < 0) {
    rt = &buffer->depth_stencil;
  } else {
    assert((unsigned int)attachment < buffer->desc.buffer_count);
    rt = buffer->render_target_list + attachment;
  }
  return rt;
}

/* Release the render target resource if it exists. */
static void
release_render_target_resource(struct render_target* rt)
{
  assert(rt);
  if(rt->resource == NULL)
    return;
  switch(rt->type) {
    case RB_RENDER_TARGET_TEXTURE2D:
      RB(tex2d_ref_put((struct rb_tex2d*)rt->resource));
      break;
    default: assert(0); break;
  }
  rt->resource = NULL;
}

static int
attach_tex2d
  (struct rb_framebuffer* buffer,
   int attachment,
   unsigned int mip,
   struct rb_tex2d* tex2d)
{
  struct render_target* rt = NULL;
  GLenum ogl3_attachment = GL_NONE;
  int err = 0;

  assert(buffer);

  if(attachment >= 0) {
    assert((unsigned int)attachment < buffer->desc.buffer_count);
    ogl3_attachment = GL_COLOR_ATTACHMENT0 + attachment;
  } else {
    switch(tex2d->format) {
      case GL_DEPTH_STENCIL:
        ogl3_attachment = GL_DEPTH_STENCIL_ATTACHMENT;
        break;
      case GL_DEPTH_COMPONENT:
        ogl3_attachment = GL_DEPTH_ATTACHMENT;
        break;
      default:
        assert(0);
        break;
    }
  }
  rt = attachment_to_render_target(buffer, attachment);
  if(!tex2d) {
    release_render_target_resource(rt);
    OGL(FramebufferTexture2D
      (GL_FRAMEBUFFER, ogl3_attachment, GL_TEXTURE_2D, 0, mip));
  } else {
    if(tex2d->mip_count < mip
    || tex2d->mip_list[mip].width != buffer->desc.width
    || tex2d->mip_list[mip].height != buffer->desc.height) {
      goto error;
    }
    release_render_target_resource(rt);
    RB(tex2d_ref_get(tex2d));
    rt->resource = tex2d;
    OGL(FramebufferTexture2D
      (GL_FRAMEBUFFER, ogl3_attachment, GL_TEXTURE_2D, tex2d->name, mip));
  }
exit:
  return err;
error:
  err = -1;
  goto exit;
}

static int
attach_render_target
  (struct rb_framebuffer* buffer,
   int attachment, /* < 0 <=> depth stencil. */
   const struct rb_render_target* render_target)
{
  int err = 0;
  assert(render_target);

  switch(render_target->type) {
    case RB_RENDER_TARGET_TEXTURE2D:
      err = attach_tex2d
        (buffer,
         attachment,
         render_target->desc.tex2d.mip_level,
         (struct rb_tex2d*)render_target->resource);
      if(err != 0)
        goto error;
      break;
    default: assert(0); break;
  }
exit:
  return err;
error:
  goto exit;
}

static void
release_framebuffer(struct ref* ref)
{
  struct rb_context* ctxt  = NULL;
  struct rb_framebuffer* buffer = NULL;
  unsigned int i = 0;
  assert(ref);

  buffer = CONTAINER_OF(ref, struct rb_framebuffer, ref);
  OGL(DeleteFramebuffers(1, &buffer->name));

  release_render_target_resource(&buffer->depth_stencil);
  for(i = 0; i < buffer->desc.buffer_count; ++i) {
    release_render_target_resource(buffer->render_target_list + i);
  }

  ctxt = buffer->ctxt;
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
  if(desc->buffer_count > RB_OGL3_MAX_COLOR_ATTACHMENTS)
    goto error;
  /* Multisampled framebuffer are not supported yet! */
  if(desc->sample_count > 1)
    goto error;

  buffer = MEM_CALLOC
    (ctxt->allocator, 1,
     sizeof(struct rb_framebuffer)
     + sizeof(struct render_target) * desc->buffer_count);
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
   const struct rb_render_target render_target_list[],
   const struct rb_render_target* depth_stencil)
{
  unsigned int i = 0;
  int err = 0;
  GLenum status = GL_FRAMEBUFFER_COMPLETE;
  bool is_bound = false;

  if(UNLIKELY
  (  !buffer
  || (count && !render_target_list)
  || count > buffer->desc.buffer_count))
    goto error;

  OGL(BindFramebuffer(GL_FRAMEBUFFER, buffer->name));
  is_bound = true;

  if(depth_stencil)
    attach_render_target(buffer, -1, depth_stencil);
  for(i = 0; i < count; ++i) {
    assert(i <= INT_MAX);
    attach_render_target(buffer, (int)i, render_target_list+i);
  }

  status = OGL(CheckFramebufferStatus(GL_FRAMEBUFFER));
  if(status != GL_FRAMEBUFFER_COMPLETE) {
    #ifndef NDEBUG
    fprintf(stderr, "framebuffer:status error: ");
    switch(status) {
      case GL_FRAMEBUFFER_UNDEFINED:
        fprintf(stderr, "undefined framebuffer\n");
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        fprintf(stderr, "incomplete attachment\n");
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        fprintf(stderr, "missing attachment\n");
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        fprintf(stderr, "incomplete draw buffer\n");
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        fprintf(stderr, "incomplete read buffer\n");
        break;
      case GL_FRAMEBUFFER_UNSUPPORTED:
        fprintf(stderr, "unsupported framebuffer\n");
        break;
      case  GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        fprintf(stderr, "incomplete multisample\n");
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        fprintf(stderr, "incomplete layer targets\n");
        break;
      default:
        fprintf(stderr, "unhandle framebuffer status\n");
        break;
    }
    #endif
    goto error;
  }

exit:
  if(is_bound) {
    OGL(BindFramebuffer
      (GL_FRAMEBUFFER, buffer->ctxt->state_cache.framebuffer_binding));
  }
  return err;
error:
  err = -1;
  goto exit;
}

