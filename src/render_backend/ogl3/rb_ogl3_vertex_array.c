#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <stdlib.h>

static const int ogl3_attrib_nb_components[] = {
  [RB_UNKNOWN_TYPE] = 0,
  [RB_FLOAT] = 1,
  [RB_FLOAT2] = 2,
  [RB_FLOAT3] = 3,
  [RB_FLOAT4] = 4
};

EXPORT_SYM int
rb_create_vertex_array
  (struct rb_context* ctxt,
   struct rb_vertex_array** out_array)
{
  struct rb_vertex_array* array = NULL;

  if(!ctxt || !out_array)
    return -1;

  array = MEM_ALLOC_I(ctxt->allocator, sizeof(struct rb_vertex_array));
  if(!array)
    return -1;

  OGL(GenVertexArrays(1, &array->name));
  *out_array = array;
  return 0;
}

EXPORT_SYM int
rb_free_vertex_array(struct rb_context* ctxt, struct rb_vertex_array* array)
{
  if(!ctxt || !array)
    return -1;

  OGL(DeleteBuffers(1, &array->name));
  MEM_FREE_I(ctxt->allocator, array);
  return 0;
}

EXPORT_SYM int
rb_bind_vertex_array(struct rb_context* ctxt, struct rb_vertex_array* array)
{
  if(!ctxt)
    return -1;

  ctxt->vertex_array_binding = array ? array->name : 0;
  OGL(BindVertexArray(ctxt->vertex_array_binding));
  return 0;
}

EXPORT_SYM int
rb_vertex_attrib_array
  (struct rb_context* ctxt,
   struct rb_vertex_array* array,
   struct rb_buffer* buffer,
   int count,
   const struct rb_buffer_attrib* attrib)
{
  intptr_t offset = 0;
  int i = 0;
  int err = 0;

  if(!ctxt
  || !array
  || !buffer
  || !attrib
  || count < 0
  || buffer->target != GL_ARRAY_BUFFER)
    goto error;

  OGL(BindVertexArray(array->name));
  OGL(BindBuffer(buffer->target, buffer->name));

  for(i=0; i < count; ++i) {

    if(attrib[i].type == RB_UNKNOWN_TYPE) {
      OGL(BindBuffer(buffer->target, ctxt->buffer_binding[buffer->binding]));
      goto error;
    }

    offset = attrib[i].offset;
    OGL(EnableVertexAttribArray(attrib[i].index));
    OGL(VertexAttribPointer
        (attrib[i].index,
         ogl3_attrib_nb_components[attrib[i].type],
         GL_FLOAT,
         GL_FALSE,
         attrib[i].stride,
         (void*)offset));
  }

  OGL(BindVertexArray(ctxt->vertex_array_binding));
  OGL(BindBuffer(buffer->target, ctxt->buffer_binding[buffer->binding]));

exit:
  return err;

error:
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_remove_vertex_attrib
  (struct rb_context* ctxt,
   struct rb_vertex_array* array,
   int count,
   int* list_of_attrib_indices)
{
  int i = 0;
  int err = 0;

  if(!ctxt
  || !array
  || count < 0
  || (count > 0 && !list_of_attrib_indices))
    return -1;

  OGL(BindVertexArray(array->name));

  for(i = 0; i < count; ++i) {
    const int current_attrib = list_of_attrib_indices[i];
    if(current_attrib < 0) {
      err = -1;
    } else {
      OGL(DisableVertexAttribArray(current_attrib));
    }
  }

  OGL(BindVertexArray(ctxt->vertex_array_binding));

  return err;
}

EXPORT_SYM int
rb_vertex_index_array
  (struct rb_context* ctxt,
   struct rb_vertex_array* array,
   struct rb_buffer* buffer)
{
  if(!ctxt || !array || (buffer && buffer->target != GL_ELEMENT_ARRAY_BUFFER))
    return -1;

  OGL(BindVertexArray(array->name));
  OGL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer ? buffer->name : 0));
  OGL(BindVertexArray(ctxt->vertex_array_binding));
  OGL(BindBuffer
      (GL_ELEMENT_ARRAY_BUFFER, ctxt->buffer_binding[RB_BIND_INDEX_BUFFER]));

  return 0;
}

