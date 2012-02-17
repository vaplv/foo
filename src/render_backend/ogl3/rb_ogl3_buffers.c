#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/ogl3/rb_ogl3_buffer.h"
#include "render_backend/ogl3/rb_ogl3_context.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct ogl3_buffer_target {
  GLenum target;
  enum rb_buffer_target binding;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static FINLINE struct ogl3_buffer_target
rb_to_ogl3_buffer_target(enum rb_buffer_target target)
{
  struct ogl3_buffer_target ogl3_target;
  memset(&ogl3_target, 0, sizeof(ogl3_target));

  switch(target) {
    case RB_BIND_VERTEX_BUFFER:
      ogl3_target.binding = RB_BIND_VERTEX_BUFFER;
      ogl3_target.target = GL_ARRAY_BUFFER;
      break;
    case RB_BIND_INDEX_BUFFER:
      ogl3_target.binding = RB_BIND_INDEX_BUFFER;
      ogl3_target.target = GL_ELEMENT_ARRAY_BUFFER;
      break;
    default:
      assert(0);
      break;
  }
  return ogl3_target;
}

static FINLINE GLenum
rb_to_ogl3_buffer_usage(enum rb_buffer_usage usage)
{
  GLenum ogl3_usage = GL_NONE;

  switch(usage) {
    case RB_BUFFER_USAGE_DEFAULT: 
      ogl3_usage = GL_DYNAMIC_DRAW;
      break;
    case RB_BUFFER_USAGE_IMMUTABLE:
      ogl3_usage = GL_STATIC_DRAW;
      break;
    case RB_BUFFER_USAGE_DYNAMIC:
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
   const struct rb_buffer_desc* desc,
   const void* init_data,
   struct rb_buffer** out_buffer)
{
  struct ogl3_buffer_target ogl3_target;
  struct rb_buffer* buffer = NULL;
  GLsizei size = 0;

  if(!ctxt
  || !desc
  || !out_buffer
  || (desc->size < 0)
  || (desc->size != (size = desc->size)))
    return -1;

  buffer = MEM_ALLOC(ctxt->allocator, sizeof(struct rb_buffer));
  if(!buffer)
    return -1;
  ref_init(&buffer->ref);
  RB(context_ref_get(ctxt));
  buffer->ctxt = ctxt;

  ogl3_target = rb_to_ogl3_buffer_target(desc->target);
  buffer->target = ogl3_target.target;
  buffer->usage = rb_to_ogl3_buffer_usage(desc->usage);
  buffer->size = (GLsizei)desc->size;
  buffer->binding = ogl3_target.binding;

  OGL(GenBuffers(1, &buffer->name));
  OGL(BindBuffer(buffer->target, buffer->name));
  OGL(BufferData(buffer->target, buffer->size, init_data, buffer->usage));
  OGL(BindBuffer(buffer->target, ctxt->buffer_binding[buffer->binding]));

  *out_buffer = buffer;
  return 0;
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
rb_bind_buffer(struct rb_context* ctxt, struct rb_buffer* buffer)
{
  const GLint name = buffer ? buffer->name : 0;

  if(!ctxt)
    return -1;

  OGL(BindBuffer(buffer->target, name));
  ctxt->buffer_binding[buffer->binding] = name;
  return 0;
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
  || (buffer->usage == rb_to_ogl3_buffer_usage(RB_BUFFER_USAGE_IMMUTABLE))
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
  OGL(BindBuffer(buffer->target,buffer->ctxt->buffer_binding[buffer->binding]));

  /* unmap == GL_FALSE must be handled by the application. TODO return a real
   * error code to differentiate this case from the error. */
  return unmap == GL_TRUE ? 0 : -1;
}

