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

struct ogl3_render_target_desc {
  GLenum format;
  GLenum type;
  size_t size;
};

struct rb_framebuffer {
  struct ref ref;
  struct rb_framebuffer_desc desc;
  struct rb_context* ctxt;
  GLuint name;
  struct rb_render_target depth_stencil;
  struct rb_render_target render_target_list[];
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
/* Release the render target resource if it exists. */
static void
release_render_target_resource(struct rb_render_target* rt)
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
  memset(rt, 0, sizeof(struct rb_render_target));
}

static int
attach_tex2d
  (struct rb_framebuffer* buffer,
   int attachment,
   const struct rb_render_target* render_target)
{
  struct rb_render_target* rt = NULL;
  struct rb_tex2d* tex2d = NULL;
  GLenum ogl3_attachment = GL_NONE;
  unsigned int mip = 0;
  int err = 0;

  assert
    (  buffer
    && (attachment < 0 || (unsigned int)attachment < buffer->desc.buffer_count)
    && render_target);

  tex2d = (struct rb_tex2d*)render_target->resource;
  mip = render_target->desc.tex2d.mip_level;
  if(attachment >= 0) {
    assert((unsigned int)attachment < buffer->desc.buffer_count);
    ogl3_attachment = GL_COLOR_ATTACHMENT0 + attachment;
    rt = buffer->render_target_list + attachment;
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
    rt = &buffer->depth_stencil;
  }
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
    OGL(FramebufferTexture2D
      (GL_FRAMEBUFFER, ogl3_attachment, GL_TEXTURE_2D, tex2d->name, mip));
  }
  memcpy(rt, render_target, sizeof(struct rb_render_target));

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
  assert
    (  buffer
    && (attachment < 0 || (unsigned int)attachment < buffer->desc.buffer_count)
    && render_target);

 switch(render_target->type) {
    case RB_RENDER_TARGET_TEXTURE2D:
      err = attach_tex2d(buffer,attachment, render_target);
      break;
    default: assert(0); break;
  }

  if(err != 0)
    goto error;

exit:
  return err;
error:
  goto exit;
}

static void
get_ogl3_render_target_desc
  (const struct rb_render_target* target,
   struct ogl3_render_target_desc* ogl3_desc)
{
  struct rb_tex2d* tex2d = NULL;

  assert(target);
  switch(target->type) {
    case RB_RENDER_TARGET_TEXTURE2D:
      tex2d = (struct rb_tex2d*)target->resource;
      ogl3_desc->format = tex2d->format;
      ogl3_desc->type = tex2d->type;
      ogl3_desc->size =
        tex2d->mip_list[target->desc.tex2d.mip_level].width
      * tex2d->mip_list[target->desc.tex2d.mip_level].height;
      break;
    default: assert(0); break;
  }
  ogl3_desc->size *= rb_ogl3_sizeof_pixel(ogl3_desc->format, ogl3_desc->type);
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
int
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
     + sizeof(struct rb_render_target) * desc->buffer_count);
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

int
rb_framebuffer_ref_get(struct rb_framebuffer* buffer)
{
  if(UNLIKELY(!buffer))
    return -1;
  ref_get(&buffer->ref);
  return 0;
}

int
rb_framebuffer_ref_put(struct rb_framebuffer* buffer)
{
  if(UNLIKELY(!buffer))
    return -1;
  ref_put(&buffer->ref, release_framebuffer);
  return 0;
}

int
rb_bind_framebuffer
  (struct rb_context* ctxt,
   struct rb_framebuffer* buffer)
{
  if(UNLIKELY(!ctxt))
    return -1;
  ctxt->state_cache.framebuffer_binding = buffer ? buffer->name : 0;
  OGL(BindFramebuffer(GL_FRAMEBUFFER, ctxt->state_cache.framebuffer_binding));
  return 0;
}

int
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

int
rb_read_back_framebuffer
  (struct rb_framebuffer* buffer,
   int rt_id,
   size_t x,
   size_t y,
   size_t width,
   size_t height,
   size_t* read_size,
   void* read_data)
{
  struct ogl3_render_target_desc desc;
  struct rb_render_target* render_target = NULL;
  int err = 0;
  memset(&desc, 0, sizeof(struct ogl3_render_target_desc));

  if(UNLIKELY
  (  !buffer
  || buffer->desc.sample_count > 1 /* not supported on mutli sampled FBO. */
  || (unsigned int)rt_id >= buffer->desc.buffer_count))
    goto error;

  /* Map the (x, y) coordinates from 'upper left' origin to OpenGL convention
   * (bottom left) */
  y = buffer->desc.height < y ? 0.f : buffer->desc.height - y;

  render_target =
    rt_id >= 0 ? buffer->render_target_list + rt_id : &buffer->depth_stencil;

  get_ogl3_render_target_desc(render_target, &desc);
  if(read_size) {
    *read_size = width * height * rb_ogl3_sizeof_pixel(desc.format, desc.type);
  }
  if(read_data) {
    OGL(BindFramebuffer(GL_FRAMEBUFFER, buffer->name));
    if(rt_id >= 0)
      OGL(ReadBuffer(GL_COLOR_ATTACHMENT0 + rt_id));

    OGL(ReadPixels(x, y, width, height, desc.format, desc.type, read_data));
    OGL(BindFramebuffer
      (GL_FRAMEBUFFER, buffer->ctxt->state_cache.framebuffer_binding));
  }

exit:
  return err;
error:
  err = -1;
  goto exit;
}

int
rb_clear_framebuffer_render_targets
  (struct rb_framebuffer* buffer,
   int clear_flag,
   unsigned int count,
   const struct rb_clear_framebuffer_color_desc* color_vals,
   float depth_val,
   char stencil_val)
{
  struct ogl3_render_target_desc rt_desc;
  int err = 0;
  int depth_stencil_flag = 0;
  memset(&rt_desc, 0, sizeof(struct ogl3_render_target_desc));

  if(UNLIKELY(!buffer))
    goto error;

  OGL(BindFramebuffer(GL_FRAMEBUFFER, buffer->name));

  /* Clear the color render targets. */
  if((clear_flag & RB_CLEAR_COLOR_BIT) != 0) {
    unsigned int i = 0;
    if(UNLIKELY(count && !color_vals))
       goto error;
    for(i = 0; i < count; ++i) {
      const unsigned rt_id = color_vals[i].index;

      if(UNLIKELY(rt_id >= buffer->desc.buffer_count))
        goto error;

      get_ogl3_render_target_desc(&buffer->render_target_list[rt_id], &rt_desc);
      if(rt_desc.type == GL_UNSIGNED_INT) {
        OGL(ClearBufferuiv(GL_COLOR, rt_id, color_vals[i].val.rgba_ui32));
      } else if(rt_desc.type == GL_INT) {
        OGL(ClearBufferiv(GL_COLOR, rt_id, color_vals[i].val.rgba_i32));
      } else { /* float type */
        OGL(ClearBufferfv(GL_COLOR, rt_id, color_vals[i].val.rgba_f));
      }
    }
  }
  /* Clear the depth stencil render target. */
  depth_stencil_flag = clear_flag & (RB_CLEAR_DEPTH_BIT | RB_CLEAR_STENCIL_BIT);
  if(depth_stencil_flag != 0) {
    get_ogl3_render_target_desc(&buffer->depth_stencil, &rt_desc);

    if(depth_stencil_flag == RB_CLEAR_DEPTH_BIT) {
      GLfloat ogl3_depth_val = depth_val;
      if(UNLIKELY(rt_desc.format != GL_DEPTH_COMPONENT))
        goto error;
      OGL(ClearBufferfv(GL_DEPTH, 0, &ogl3_depth_val));
    } else if(depth_stencil_flag == RB_CLEAR_STENCIL_BIT) {
      GLint ogl3_stencil_val = stencil_val;
      if(UNLIKELY(rt_desc.format != GL_DEPTH_STENCIL))
        goto error;
      OGL(ClearBufferiv(GL_STENCIL, 0, &ogl3_stencil_val));
    } else { /* depth_stencil_flag == RB_CLEAR_DEPTH_BIT|RB_CLEAR_STENCIL_BIT */
      if(UNLIKELY(rt_desc.format != GL_DEPTH_STENCIL))
        goto error;
      OGL(ClearBufferfi(GL_DEPTH_STENCIL, 0, depth_val, (GLint)stencil_val));
    }
  }

exit:
  OGL(BindFramebuffer
    (GL_FRAMEBUFFER, buffer->ctxt->state_cache.framebuffer_binding));
  return err;
error:
  err = -1;
  goto exit;
}

