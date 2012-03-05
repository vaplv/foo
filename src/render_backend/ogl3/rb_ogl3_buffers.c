#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/ogl3/rb_ogl3_buffers.h"
#include "render_backend/ogl3/rb_ogl3_context.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static FINLINE enum rb_ogl3_buffer_target
public_to_private_rb_target(enum rb_buffer_target public_target) 
{
  enum rb_ogl3_buffer_target private_target = RB_OGL3_NB_BUFFER_TARGETS;
  switch(public_target) {
    case RB_BIND_VERTEX_BUFFER:
      private_target = RB_OGL3_BIND_VERTEX_BUFFER;
      break;
    case RB_BIND_INDEX_BUFFER:
      private_target = RB_OGL3_BIND_INDEX_BUFFER;
      break;
    default:
      assert(0);
      break;
  }
  return private_target;
}

static FINLINE GLenum
rb_to_ogl3_buffer_target(enum rb_ogl3_buffer_target target)
{
  GLenum ogl3_target = GL_NONE;
  switch(target) {
    case RB_OGL3_BIND_VERTEX_BUFFER:
      ogl3_target = GL_ARRAY_BUFFER;
      break;
    case RB_OGL3_BIND_INDEX_BUFFER:
      ogl3_target = GL_ELEMENT_ARRAY_BUFFER;
      break;
    case RB_OGL3_BIND_PIXEL_READBACK_BUFFER:
      ogl3_target = GL_PIXEL_PACK_BUFFER;
      break;
    case RB_OGL3_BIND_PIXEL_DOWNLOAD_BUFFER:
      ogl3_target = GL_PIXEL_UNPACK_BUFFER;
      break;
    default:
      assert(0);
      break;
  }
  return ogl3_target;
}

static FINLINE GLenum
rb_to_ogl3_usage(enum rb_usage usage)
{
  GLenum ogl3_usage = GL_NONE;

  switch(usage) {
    case RB_USAGE_DEFAULT:
      ogl3_usage = GL_DYNAMIC_DRAW;
      break;
    case RB_USAGE_IMMUTABLE:
      ogl3_usage = GL_STATIC_DRAW;
      break;
    case RB_USAGE_DYNAMIC:
      ogl3_usage = GL_STREAM_DRAW;
      break;
    default:
      assert(0);
      break;
  }
  return ogl3_usage;
}

static void
release_buffer(struct ref* ref)
{
  struct rb_buffer* buffer = NULL;
  struct rb_context* ctxt = NULL;
  assert(ref);

  buffer = CONTAINER_OF(ref, struct rb_buffer, ref);
  ctxt = buffer->ctxt;

  if(buffer->name == ctxt->state_cache.buffer_binding[buffer->binding])
    OGL(BindBuffer(buffer->target, 0));
 
  OGL(DeleteBuffers(1, &buffer->name));
  MEM_FREE(ctxt->allocator, buffer);
  RB(context_ref_put(ctxt));
}

/*******************************************************************************
 *
 * Buffer functions.
 *
 ******************************************************************************/
EXPORT_SYM int
rb_create_buffer
  (struct rb_context* ctxt,
   const struct rb_buffer_desc* public_desc,
   const void* init_data,
   struct rb_buffer** out_buffer)
{
  const struct rb_ogl3_buffer_desc private_desc = {
    .size = public_desc->size,
    .target = public_to_private_rb_target(public_desc->target),
    .usage = public_desc->usage
  };
  return rb_ogl3_create_buffer(ctxt, &private_desc,init_data, out_buffer);
}

EXPORT_SYM int
rb_buffer_ref_get(struct rb_buffer* buffer)
{
  if(!buffer)
    return -1;
  ref_get(&buffer->ref);
  return 0;
}

EXPORT_SYM int
rb_buffer_ref_put(struct rb_buffer* buffer)
{
  if(!buffer)
    return -1;
  ref_put(&buffer->ref, release_buffer);
  return 0;
}

EXPORT_SYM int
rb_bind_buffer
  (struct rb_context* ctxt,
   struct rb_buffer* buffer, 
   enum rb_buffer_target target)
{
  return rb_ogl3_bind_buffer(ctxt, buffer, public_to_private_rb_target(target));
}

EXPORT_SYM int
rb_buffer_data
  (struct rb_buffer* buffer,
   int offset,
   int size,
   const void* data)
{
  void* mapped_mem = NULL;
  GLboolean unmap = GL_FALSE;

  if(!buffer
  || (offset < 0)
  || (size < 0)
  || (size != 0 && !data)
  || (buffer->size < offset + size))
    return -1;

  if(size == 0)
    return 0;

  OGL(BindBuffer(buffer->target, buffer->name));

  if(offset == 0 && size == buffer->size) {
    mapped_mem = OGL(MapBuffer(buffer->target, GL_WRITE_ONLY));
  } else {
    const GLbitfield access = GL_MAP_WRITE_BIT;
    mapped_mem = OGL(MapBufferRange(buffer->target, offset, size, access));
  }
  assert(mapped_mem != NULL);
  memcpy(mapped_mem, data, size);
  unmap = OGL(UnmapBuffer(buffer->target));
  OGL(BindBuffer
    (buffer->target, 
     buffer->ctxt->state_cache.buffer_binding[buffer->binding]));

  /* unmap == GL_FALSE must be handled by the application. TODO return a real
   * error code to differentiate this case from the error. */
  return unmap == GL_TRUE ? 0 : -1;
}

/*******************************************************************************
 *
 * Private functions.
 *
 ******************************************************************************/
int
rb_ogl3_create_buffer
  (struct rb_context* ctxt,
   const struct rb_ogl3_buffer_desc* desc,
   const void* init_data,
   struct rb_buffer** out_buffer)
{
  struct rb_buffer* buffer = NULL;

  if(!ctxt
  || !desc
  || !out_buffer
  || (desc->target == RB_OGL3_NB_BUFFER_TARGETS)
  || (desc->usage == RB_USAGE_IMMUTABLE && init_data == NULL))
    return -1;

  buffer = MEM_ALLOC(ctxt->allocator, sizeof(struct rb_buffer));
  if(!buffer)
    return -1;
  ref_init(&buffer->ref);
  RB(context_ref_get(ctxt));
  buffer->ctxt = ctxt;

  buffer->target = rb_to_ogl3_buffer_target(desc->target);
  buffer->usage = rb_to_ogl3_usage(desc->usage);
  buffer->size = (GLsizei)desc->size;
  buffer->binding = desc->target;

  OGL(GenBuffers(1, &buffer->name));
  OGL(BindBuffer(buffer->target, buffer->name));
  OGL(BufferData(buffer->target, buffer->size, init_data, buffer->usage));
  OGL(BindBuffer
    (buffer->target, 
     ctxt->state_cache.buffer_binding[buffer->binding]));

  *out_buffer = buffer;
  return 0;
}

int
rb_ogl3_bind_buffer
  (struct rb_context* ctxt,
   struct rb_buffer* buffer,
   enum rb_ogl3_buffer_target target)
{
  GLenum current_name = 0;
  GLenum name = 0;
  int err = 0;

  if(!ctxt || (buffer && (buffer->binding != target)))
    goto error;

  current_name = ctxt->state_cache.buffer_binding[target];
  name = buffer ? buffer->name : 0;

  if(current_name != name) {
    OGL(BindBuffer(rb_to_ogl3_buffer_target(target), name));
    ctxt->state_cache.buffer_binding[target] = name;
  }

exit:
  return err;
error:
  err = -1;
  goto exit;
}


