#include "render_backend/ogl3/rb_ogl3.h"
#include "sys/sys.h"
#include <stdlib.h>

static int ogl3_attrib_nb_components[] = {
  [RB_ATTRIB_FLOAT] = 1,
  [RB_ATTRIB_FLOAT2] = 2,
  [RB_ATTRIB_FLOAT3] = 3,
  [RB_ATTRIB_FLOAT4] = 4
};

EXPORT_SYM int
rb_create_vertex_array
  (struct rb_context* ctxt,
   struct rb_vertex_array** out_array)
{
  struct rb_vertex_array* array = NULL;

  if(!ctxt || !out_array)
    return -1;

  array = malloc(sizeof(struct rb_vertex_array));
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
  free(array);
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
  int i = 0;
  intptr_t offset = 0;

  if(!ctxt
  || !array
  || !buffer
  || !attrib
  || count < 0
  || buffer->target != GL_ARRAY_BUFFER
  || attrib->offset < 0)
    return -1;

  OGL(BindVertexArray(array->name));
  OGL(BindBuffer(buffer->target, buffer->name));

  for(i=0; i < count; ++i) {
    offset = attrib[i].offset;
    OGL(EnableVertexAttribArray(attrib[i].index));
    OGL(VertexAttribPointer
        (attrib[i].index,
         ogl3_attrib_nb_components[attrib[i].format],
         GL_FLOAT,
         GL_FALSE,
         attrib[i].stride,
         (void*)offset));
  }
  OGL(BindVertexArray(ctxt->vertex_array_binding));
  OGL(BindBuffer(buffer->target, ctxt->buffer_binding[buffer->bind_id]));

  return 0;
}

EXPORT_SYM int
rb_vertex_index_array
  (struct rb_context* ctxt,
   struct rb_vertex_array* array,
   struct rb_buffer* buffer)
{
  if(!ctxt || !array || !buffer || buffer->target != GL_ELEMENT_ARRAY_BUFFER)
    return -1;

  OGL(BindVertexArray(array->name));
  OGL(BindBuffer(buffer->target, buffer->name));
  OGL(BindVertexArray(ctxt->vertex_array_binding));
  OGL(BindBuffer(buffer->target, ctxt->buffer_binding[buffer->bind_id]));

  return 0;
}

