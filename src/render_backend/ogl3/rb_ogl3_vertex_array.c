#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/ogl3/rb_ogl3_buffers.h"
#include "render_backend/ogl3/rb_ogl3_context.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>

struct rb_vertex_array {
  struct ref ref;
  struct rb_context* ctxt;
  GLuint name;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static FINLINE int 
ogl3_attrib_nb_components(enum rb_type type)
{
  int nb = 0;
  switch(type) {
    case RB_UNKNOWN_TYPE: nb = 0; break;
    case RB_FLOAT: nb = 1; break;
    case RB_FLOAT2: nb = 2; break;
    case RB_FLOAT3: nb = 3; break;
    case RB_FLOAT4: nb = 4; break;
    default:
      assert(0);
      break;
  }
  return nb;
}

static void
release_vertex_array(struct ref* ref)
{
  struct rb_context* ctxt = NULL;
  struct rb_vertex_array* varray = NULL;
  assert(ref);

  varray = CONTAINER_OF(ref, struct rb_vertex_array, ref);
  ctxt = varray->ctxt;

  if(ctxt->state_cache.vertex_array_binding == varray->name)
    RB(bind_vertex_array(ctxt, NULL));

  OGL(DeleteVertexArrays(1, &varray->name));
  MEM_FREE(ctxt->allocator, varray);
  RB(context_ref_put(ctxt));
}

/*******************************************************************************
 *
 * Vertex array functions.
 *
 ******************************************************************************/
EXPORT_SYM int
rb_create_vertex_array
  (struct rb_context* ctxt,
   struct rb_vertex_array** out_array)
{
  struct rb_vertex_array* array = NULL;

  if(!ctxt || !out_array)
    return -1;

  array = MEM_ALLOC(ctxt->allocator, sizeof(struct rb_vertex_array));
  if(!array)
    return -1;
  ref_init(&array->ref);
  RB(context_ref_get(ctxt));
  array->ctxt = ctxt;

  OGL(GenVertexArrays(1, &array->name));
  *out_array = array;
  return 0;
}

EXPORT_SYM int
rb_vertex_array_ref_get(struct rb_vertex_array* array)
{
  if(!array)
    return -1;
  ref_get(&array->ref);
  return 0;
}

EXPORT_SYM int
rb_vertex_array_ref_put(struct rb_vertex_array* array)
{
  if(!array)
    return -1;
  ref_put(&array->ref, release_vertex_array);
  return 0;
}

EXPORT_SYM int
rb_bind_vertex_array(struct rb_context* ctxt, struct rb_vertex_array* array)
{
  if(!ctxt)
    return -1;
  ctxt->state_cache.vertex_array_binding = array ? array->name : 0;
  OGL(BindVertexArray(ctxt->state_cache.vertex_array_binding));
  return 0;
}

EXPORT_SYM int
rb_vertex_attrib_array
  (struct rb_vertex_array* array,
   struct rb_buffer* buffer,
   int count,
   const struct rb_buffer_attrib* attrib)
{
  intptr_t offset = 0;
  int i = 0;
  int err = 0;

  if(!array
  || !buffer
  || !attrib
  || count < 0
  || buffer->target != GL_ARRAY_BUFFER)
    goto error;

  OGL(BindVertexArray(array->name));
  OGL(BindBuffer(buffer->target, buffer->name));

  for(i=0; i < count; ++i) {

    if(attrib[i].type == RB_UNKNOWN_TYPE) {
      OGL(BindBuffer
        (buffer->target, 
         array->ctxt->state_cache.buffer_binding[buffer->binding]));
      goto error;
    }

    offset = attrib[i].offset;
    OGL(EnableVertexAttribArray(attrib[i].index));
    OGL(VertexAttribPointer
        (attrib[i].index,
         ogl3_attrib_nb_components(attrib[i].type),
         GL_FLOAT,
         GL_FALSE,
         attrib[i].stride,
         (void*)offset));
  }

  OGL(BindVertexArray(array->ctxt->state_cache.vertex_array_binding));
  OGL(BindBuffer
    (buffer->target, array->ctxt->state_cache.buffer_binding[buffer->binding]));

exit:
  return err;

error:
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_remove_vertex_attrib
  (struct rb_vertex_array* array,
   int count,
   const int* list_of_attrib_indices)
{
  int i = 0;
  int err = 0;

  if(!array
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
  OGL(BindVertexArray(array->ctxt->state_cache.vertex_array_binding));

  return err;
}

EXPORT_SYM int
rb_vertex_index_array(struct rb_vertex_array* array, struct rb_buffer* buffer)
{
  if(!array || (buffer && buffer->target != GL_ELEMENT_ARRAY_BUFFER))
    return -1;

  OGL(BindVertexArray(array->name));
  OGL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer ? buffer->name : 0));
  OGL(BindVertexArray(array->ctxt->state_cache.vertex_array_binding));
  OGL(BindBuffer
    (GL_ELEMENT_ARRAY_BUFFER,
     array->ctxt->state_cache.buffer_binding[RB_OGL3_BIND_INDEX_BUFFER]));

  return 0;
}

