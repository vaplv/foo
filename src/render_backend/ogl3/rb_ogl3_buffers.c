#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <stdlib.h>
#include <string.h>

struct ogl3_buffer_target {
  GLenum target;
  enum rb_buffer_target binding;
};

static const struct ogl3_buffer_target rb_to_ogl3_buffer_target[] = {
  [RB_BIND_VERTEX_BUFFER] = {
    .binding = RB_BIND_VERTEX_BUFFER,
    .target = GL_ARRAY_BUFFER
  },
  [RB_BIND_INDEX_BUFFER] = {
    .binding = RB_BIND_INDEX_BUFFER,
    .target = GL_ELEMENT_ARRAY_BUFFER
  }
};

static const GLenum rb_to_ogl3_buffer_usage[] = {
  [RB_BUFFER_USAGE_DEFAULT] = GL_DYNAMIC_DRAW,
  [RB_BUFFER_USAGE_IMMUTABLE] = GL_STATIC_DRAW,
  [RB_BUFFER_USAGE_DYNAMIC] = GL_STREAM_DRAW
};

EXPORT_SYM int
rb_create_buffer
  (struct rb_context* ctxt,
   const struct rb_buffer_desc* desc,
   const void* init_data,
   struct rb_buffer** out_buffer)
{
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

  buffer->target = rb_to_ogl3_buffer_target[desc->target].target;
  buffer->usage = rb_to_ogl3_buffer_usage[desc->usage];
  buffer->size = (GLsizei)desc->size;
  buffer->binding = rb_to_ogl3_buffer_target[desc->target].binding;

  OGL(GenBuffers(1, &buffer->name));
  OGL(BindBuffer(buffer->target, buffer->name));
  OGL(BufferData(buffer->target, buffer->size, init_data, buffer->usage));
  OGL(BindBuffer(buffer->target, ctxt->buffer_binding[buffer->binding]));

  *out_buffer = buffer;
  return 0;
}

EXPORT_SYM int
rb_free_buffer(struct rb_context* ctxt, struct rb_buffer* buffer)
{
  if(!ctxt || !buffer)
    return -1;

  OGL(DeleteBuffers(1, &buffer->name));
  MEM_FREE(ctxt->allocator, buffer);
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
  (struct rb_context* ctxt,
   struct rb_buffer* buffer,
   int offset,
   int size,
   const void* data)
{
  void* mapped_mem = NULL;
  GLboolean unmap = GL_FALSE;

  if(!ctxt
  || !buffer
  || (offset < 0)
  || (size < 0)
  || (size != 0 && !data)
  || (buffer->usage == rb_to_ogl3_buffer_usage[RB_BUFFER_USAGE_IMMUTABLE])
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
  OGL(BindBuffer(buffer->target, ctxt->buffer_binding[buffer->binding]));

  /* unmap == GL_FALSE must be handled by the application. TODO return a real
   * error code to differentiate this case from the error. */
  return unmap == GL_TRUE ? 0 : -1;
}

