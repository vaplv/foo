#include "render_backend/ogl3/rb_ogl3.h"
#include "render_backend/rb.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
#define ATTRIB_VALUE(suffix)\
  static void\
  attrib_##suffix(GLuint index, const void* data)\
  {\
    OGL(VertexAttrib##suffix(index, data));\
  }\

ATTRIB_VALUE(1fv)
ATTRIB_VALUE(2fv)
ATTRIB_VALUE(3fv)
ATTRIB_VALUE(4fv)

static void
(*get_attrib_setter(GLenum attrib_type))(GLuint, const void*)
{
  switch(attrib_type) {
    case GL_FLOAT: return &attrib_1fv;
    case GL_FLOAT_VEC2: return &attrib_2fv;
    case GL_FLOAT_VEC3: return &attrib_3fv;
    case GL_FLOAT_VEC4: return &attrib_4fv;
    default: return NULL;
  }
}

static int
get_active_attrib
  (struct rb_context* ctxt,
   struct rb_program* program,
   GLuint index,
   GLsizei bufsize,
   GLchar* buffer,
   struct rb_attrib** out_attrib)
{
  struct rb_attrib* attr = NULL;
  GLsizei attr_namelen = 0;
  GLint attr_size = 0;
  GLenum attr_type;
  int err = 0;

  if(!ctxt
  || !program
  || !out_attrib
  || bufsize < 0
  || (bufsize > 0 && !buffer))
    goto error;

  OGL(GetActiveAttrib
      (program->name,
       index,
       bufsize,
       &attr_namelen,
       &attr_size,
       &attr_type,
       buffer));

  attr = MEM_CALLOC(ctxt->allocator, 1, sizeof(struct rb_attrib));
  if(!attr)
    goto error;

  attr->program = program;
  attr->program->ref_count += 1;
  attr->type = attr_type;
  attr->set = get_attrib_setter(attr_type);

  if(buffer) {
    /* Add 1 to namelen <=> include the null character. */
    ++attr_namelen;

    attr->name = MEM_ALLOC(ctxt->allocator, sizeof(char) * attr_namelen);
    if(!attr->name)
      goto error;

    attr->name = strncpy(attr->name, buffer, attr_namelen);
    attr->index = OGL(GetAttribLocation(program->name, attr->name));
  }

exit:
  *out_attrib = attr;
  return err;

error:
  if(attr)
    rb_release_attribs(ctxt, 1, &attr);

  attr = NULL;
  err = -1;
  goto exit;

}

/*******************************************************************************
 *
 * Attrib implementation.
 *
 ******************************************************************************/
EXPORT_SYM int
rb_get_attribs
  (struct rb_context* ctxt,
   struct rb_program* prog,
   size_t* out_nb_attribs,
   struct rb_attrib* dst_attrib_list[])
{
  GLchar* attr_buffer = NULL;
  int attr_buflen = 0;
  int nb_attribs = 0;
  int attr_id = 0;
  int err = 0;

  if(!ctxt || !prog || !out_nb_attribs)
    goto error;

  if(!prog->is_linked)
    goto error;

  OGL(GetProgramiv(prog->name, GL_ACTIVE_ATTRIBUTES, &nb_attribs));
  assert(nb_attribs >= 0);

  if(dst_attrib_list) {
    OGL(GetProgramiv(prog->name, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &attr_buflen));
    attr_buffer = MEM_ALLOC(ctxt->allocator, sizeof(GLchar) * attr_buflen);
    if(!attr_buffer)
      goto error;

    for(attr_id = 0; attr_id < nb_attribs; ++attr_id) {
      struct rb_attrib* attr = NULL;

      err = get_active_attrib
        (ctxt, prog, attr_id, attr_buflen, attr_buffer, &attr);
      if(err != 0)
        goto error;

      dst_attrib_list[attr_id] = attr;
    }
  }

exit:
  if(attr_buffer)
    MEM_FREE(ctxt->allocator, attr_buffer);
  if(out_nb_attribs)
    *out_nb_attribs = nb_attribs;
  return err;

error:
  if(dst_attrib_list) {
    /* NOTE: attr_id <=> nb attribs in dst_attrib_list; */
    rb_release_attribs(ctxt, attr_id, dst_attrib_list);
  }
  nb_attribs = 0;
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_get_named_attrib
  (struct rb_context* ctxt,
   struct rb_program* prog,
   const char* name,
   struct rb_attrib** out_attrib)
{
  struct rb_attrib* attr = NULL;
  GLint attr_id = 0;
  size_t name_len = 0;
  int err = 0;

  if(!ctxt || !prog || !name || !out_attrib)
    goto error;

  if(!prog->is_linked)
    goto error;

  attr_id = OGL(GetAttribLocation(prog->name, name));
  if(attr_id == -1)
    goto error;

  err = get_active_attrib(ctxt, prog, attr_id, 0, NULL, &attr);
  if(err != 0)
    goto error;

  name_len = strlen(name) + 1;
  attr->name = MEM_ALLOC(ctxt->allocator, sizeof(char) * name_len);
  if(!attr->name)
    goto error;

  attr->name = strncpy(attr->name, name, name_len);
  attr->index = OGL(GetAttribLocation(prog->name, attr->name));

exit:
  *out_attrib = attr;
  return err;

error:
  if(attr)
    rb_release_attribs(ctxt, 1, &attr);

  attr = NULL;
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_release_attribs
  (struct rb_context* ctxt,
   size_t nb_attribs,
   struct rb_attrib* attrib_list[])
{
  size_t i = 0;
  int err = 0;

  if(!ctxt || (nb_attribs && !attrib_list))
    goto error;

  for(i = 0; i < nb_attribs; ++i) {
    struct rb_attrib* attr = attrib_list[i];
    if(!attr) {
      err = -1;
    } else {
      assert(attr->name);
      assert(attr->program->ref_count > 0);

      attr->program->ref_count -= 1;
      MEM_FREE(ctxt->allocator, attr->name);
      MEM_FREE(ctxt->allocator, attr);
    }
  }

exit:
  return err;

error:
  err = -1;
  goto exit;
}

EXPORT_SYM int
rb_attrib_data
  (struct rb_context* ctxt,
   struct rb_attrib* attr,
   const void* data)
{
  if(!ctxt || !attr || !data)
    return -1;

  assert(attr->set != NULL);
  OGL(UseProgram(attr->program->name));
  attr->set(attr->index, data);
  OGL(UseProgram(ctxt->current_program));
  return 0;
}

EXPORT_SYM int
rb_get_attrib_desc
  (struct rb_context* ctxt,
   const struct rb_attrib* attr,
   struct rb_attrib_desc* desc)
{
  if(!ctxt || !attr || !desc)
    return -1;

  desc->name = attr->name;
  desc->index = attr->index;
  desc->type = ogl3_to_rb_type(attr->type);
  return 0;
}

