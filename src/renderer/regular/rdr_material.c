#include "renderer/regular/rdr_attrib_c.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_material_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr_material.h"
#include "stdlib/sl_linked_list.h"
#include "sys/sys.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const enum rb_shader_type rdr_shader_to_rb_shader_type[] = {
  [RDR_VERTEX_SHADER] = RB_VERTEX_SHADER,
  [RDR_GEOMETRY_SHADER] = RB_GEOMETRY_SHADER,
  [RDR_FRAGMENT_SHADER] = RB_FRAGMENT_SHADER
};

struct material {
  struct rb_program* program;
  struct rb_shader* shader_list[RDR_NB_SHADER_USAGES];
  struct rb_attrib** attrib_list;
  struct rb_uniform** uniform_list;
  char* log;
  struct sl_linked_list* callback_list;
  size_t nb_attribs;
  size_t nb_uniforms;
  bool is_linked;
};

/*******************************************************************************
 *
 *  Helper functions.
 *
 ******************************************************************************/
static void
release_shaders
  (struct rdr_system* sys,
   struct rb_program* program,
   struct rb_shader* shader_list[RDR_NB_SHADER_USAGES])
{
  size_t i = 0;
  int err = 0;

  assert(shader_list);

  for(i = 0; i < RDR_NB_SHADER_USAGES; ++i) {
    struct rb_shader* shader = shader_list[i];

    if(shader !=NULL) {
      int is_attached = 0;
      err = sys->rb.is_shader_attached(sys->ctxt, shader, &is_attached);
      assert(err == 0);
      if(is_attached != 0) {
        err = sys->rb.detach_shader(sys->ctxt, program, shader);
        assert(err == 0);
      }
      err = sys->rb.free_shader(sys->ctxt, shader);
      assert(err == 0);
    }
    shader_list[i] = NULL;
  }
}

static enum rdr_error
build_program
  (struct rdr_system* sys,
   struct rb_program* program,
   const char* shader_source_list[RDR_NB_SHADER_USAGES],
   struct rb_shader* shader_list[RDR_NB_SHADER_USAGES],
   char** out_log,
   bool *out_is_linked)
{
  const char* log = NULL;
  size_t nb_created_shaders = 0;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  assert(sys && shader_source_list && out_log && out_is_linked);

  *out_log = NULL;
  *out_is_linked = false;

  for(i = 0, nb_created_shaders = 0; i < RDR_NB_SHADER_USAGES; ++i) {
    const char* src = shader_source_list[i];

    assert(shader_list[i] == NULL);

    if(src != NULL) {
      struct rb_shader* shader = NULL;
      enum rb_shader_type type = rdr_shader_to_rb_shader_type[i];

      err = sys->rb.create_shader(sys->ctxt, type, src, strlen(src), &shader);
      if(err != 0) {
        err = sys->rb.get_shader_log(sys->ctxt, shader, &log);
        if(err == 0) {
          char* tmp_log = NULL;
          
          assert(log != NULL);
          tmp_log = malloc(strlen(log) + 1);
          if(!tmp_log) {
            rdr_err = RDR_MEMORY_ERROR;
            goto error;
          }
          tmp_log = strncpy(tmp_log, log, strlen(log) + 1);
          *out_log = tmp_log;
        }
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }

      err = sys->rb.attach_shader(sys->ctxt, program, shader);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }
      shader_list[i] = shader;
      ++nb_created_shaders;
    }
  }

  err = sys->rb.link_program(sys->ctxt, program);
  if(err != 0) {
    err = sys->rb.get_program_log(sys->ctxt, program, &log);
    if(err == 0) {
      char* tmp_log = NULL; 

      assert(log != NULL);
      tmp_log = malloc(strlen(log) + 1);
      if(!tmp_log) {
        rdr_err = RDR_MEMORY_ERROR;
        goto error;
      }
      tmp_log = strncpy(tmp_log, log, strlen(log) + 1);
      *out_log = tmp_log;
    }
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

  *out_is_linked = true;

exit:
  return rdr_err;

error:
  release_shaders(sys, program, shader_list);
  goto exit;
}

static enum rdr_error
get_attribs
  (struct rdr_system* sys,
   struct rb_program* program,
   size_t* out_nb_attribs,
   struct rb_attrib** out_attrib_list[])

{
  struct rb_attrib** mtr_attrib_list = NULL;
  size_t nb_mtr_attribs = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  assert(sys && out_nb_attribs && out_attrib_list);

  err = sys->rb.get_attribs(sys->ctxt, program, &nb_mtr_attribs, NULL);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

  if(nb_mtr_attribs) {
    mtr_attrib_list = malloc(sizeof(struct rb_attrib*) * nb_mtr_attribs);
    if(!mtr_attrib_list) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }

    err = sys->rb.get_attribs
      (sys->ctxt, program, &nb_mtr_attribs, mtr_attrib_list);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }
  }

exit:
  *out_attrib_list = mtr_attrib_list;
  *out_nb_attribs = nb_mtr_attribs;
  return rdr_err;

error:
  if(sys && mtr_attrib_list) {
    sys->rb.release_attribs(sys->ctxt, nb_mtr_attribs, mtr_attrib_list);
    free(mtr_attrib_list);
  }
  mtr_attrib_list = NULL;
  nb_mtr_attribs = 0;
  goto exit;
}

static void
release_attribs
  (struct rdr_system* sys,
   size_t nb_attribs,
   struct rb_attrib* attrib_list[])
{
  int err = 0;

  assert(sys);

  if(nb_attribs) {
    assert(attrib_list != NULL);

    err = sys->rb.release_attribs
      (sys->ctxt, nb_attribs, attrib_list);
    assert(err == 0);

    free(attrib_list);
  }
}

static enum rdr_error
get_uniforms
  (struct rdr_system* sys,
   struct rb_program* program,
   size_t* out_nb_uniforms,
   struct rb_uniform** out_uniform_list[])
{
  struct rb_uniform** uniform_list = NULL;
  size_t nb_uniforms = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  assert(sys && out_nb_uniforms && out_uniform_list);

  err = sys->rb.get_uniforms(sys->ctxt, program, &nb_uniforms, NULL);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

  if(nb_uniforms) {
    uniform_list = malloc(sizeof(struct rb_uniform*) * nb_uniforms);
    if(!uniform_list) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }

    err = sys->rb.get_uniforms(sys->ctxt, program, &nb_uniforms, uniform_list);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }
  }

exit:
  *out_uniform_list = uniform_list;
  *out_nb_uniforms = nb_uniforms;
  return rdr_err;

error:
  if(sys && uniform_list) {
    sys->rb.release_uniforms(sys->ctxt, nb_uniforms, uniform_list);
    free(uniform_list);
  }
  uniform_list = NULL;
  nb_uniforms = 0;
  goto exit;
}

static void
release_uniforms
  (struct rdr_system* sys,
   size_t nb_uniforms,
   struct rb_uniform* uniform_list[])
{
  int err = 0;

  assert(sys);

  if(nb_uniforms) {
    assert(uniform_list != NULL);

    err = sys->rb.release_uniforms
      (sys->ctxt, nb_uniforms, uniform_list);
    assert(err == 0);

    free(uniform_list);
  }
}

static enum rdr_error
invoke_callbacks(struct rdr_system* sys, struct rdr_material* mtr_obj)
{
  struct sl_node* node = NULL;
  struct material* mtr = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(sys && mtr_obj);

  mtr = RDR_GET_OBJECT_DATA(sys, mtr_obj);

  for(SL(linked_list_head(sys->sl_ctxt, mtr->callback_list, &node));
      node != NULL;
      SL(next_node(sys->sl_ctxt, node, &node))) {
    struct rdr_material_callback_desc* desc = NULL;
    enum rdr_error last_err = RDR_NO_ERROR;

    SL(node_data(sys->sl_ctxt, node, (void**)&desc));

    last_err = desc->func(sys, mtr_obj, desc->data);
    if(last_err != RDR_NO_ERROR)
      rdr_err = last_err;
  }

  return rdr_err;
}

static void
free_material(struct rdr_system* sys, void* data)
{
  struct material* mtr = data;
  int err = 0;

  assert(sys && mtr);

  if(mtr->callback_list) {
    struct sl_node* node = NULL;

    SL(linked_list_head(sys->sl_ctxt, mtr->callback_list, &node));
    assert(node == NULL);
    SL(free_linked_list(sys->sl_ctxt, mtr->callback_list));
  }

  release_attribs(sys, mtr->nb_attribs, mtr->attrib_list);
  release_uniforms(sys, mtr->nb_uniforms, mtr->uniform_list);

  if(mtr->log != NULL) {
    free(mtr->log);
    mtr->log = NULL;
  }

  if(mtr->program) {
    release_shaders(sys, mtr->program, mtr->shader_list);

    err = sys->rb.free_program(sys->ctxt, mtr->program);
    assert(err == 0);
  }
}

/*******************************************************************************
 *
 * Public render material functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rdr_error
rdr_create_material(struct rdr_system* sys, struct rdr_material** out_mtr_obj)
{
  struct rdr_material* mtr_obj = NULL;
  struct material* mtr = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  int err = 0;

  if(!sys || !out_mtr_obj) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  rdr_err = RDR_CREATE_OBJECT
    (sys, sizeof(struct material), free_material, mtr_obj);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  mtr = RDR_GET_OBJECT_DATA(sys, mtr_obj);

  err = sys->rb.create_program(sys->ctxt, &mtr->program);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

  sl_err = sl_create_linked_list
    (sys->sl_ctxt,
     sizeof(struct rdr_material_callback_desc),
     ALIGNOF(struct rdr_material_callback_desc),
     &mtr->callback_list);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

exit:
  if(out_mtr_obj)
    *out_mtr_obj = mtr_obj;
  return rdr_err;

error:
  if(mtr_obj) {
    while(RDR_RELEASE_OBJECT(sys, mtr_obj));
    mtr_obj = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_free_material(struct rdr_system* sys, struct rdr_material* mtr)
{
  if(!sys || !mtr)
    return RDR_INVALID_ARGUMENT;

  RDR_RELEASE_OBJECT(sys, mtr);

  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_get_material_log
  (struct rdr_system* sys,
   struct rdr_material* mtr_obj,
   const char** out_log)
{
  struct material* mtr = NULL;

  if(!sys || !mtr_obj || !out_log)
    return RDR_INVALID_ARGUMENT;

  mtr = RDR_GET_OBJECT_DATA(sys, mtr_obj);
  *out_log = mtr->log;

  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_material_program
  (struct rdr_system* sys,
   struct rdr_material* mtr_obj,
   const char* shader_sources[RDR_NB_SHADER_USAGES])
{
  struct material* mtr = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum rdr_error tmp_err = RDR_NO_ERROR;

  if(!sys || !mtr_obj || !shader_sources) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  mtr = RDR_GET_OBJECT_DATA(sys, mtr_obj);

  release_attribs(sys, mtr->nb_attribs, mtr->attrib_list);
  mtr->nb_attribs = 0;
  mtr->attrib_list = NULL;

  release_uniforms(sys, mtr->nb_uniforms, mtr->uniform_list);
  mtr->nb_uniforms = 0;
  mtr->uniform_list = NULL;

  release_shaders(sys, mtr->program, mtr->shader_list);

  if(mtr->log) {
    free(mtr->log);
    mtr->log = NULL;
  }

  rdr_err = build_program
    (sys, 
     mtr->program, 
     shader_sources, 
     mtr->shader_list, 
     &mtr->log, 
     &mtr->is_linked);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  if(mtr->is_linked) {
    rdr_err = get_attribs
      (sys, mtr->program, &mtr->nb_attribs, &mtr->attrib_list);
    if(rdr_err != RDR_NO_ERROR)
      goto error;

    rdr_err = get_uniforms
      (sys, mtr->program, &mtr->nb_uniforms, &mtr->uniform_list);
    if(rdr_err != RDR_NO_ERROR)
      goto error;
  }

exit:
  if(sys && mtr_obj) {
    tmp_err = invoke_callbacks(sys, mtr_obj);
    if(tmp_err != RDR_NO_ERROR)
      rdr_err = tmp_err;
  }
  return rdr_err;

error:
  goto exit;
}

/*******************************************************************************
 *
 * Private render material functions.
 *
 ******************************************************************************/
enum rdr_error
rdr_bind_material(struct rdr_system* sys, struct rdr_material* mtr_obj)
{
  struct rb_program* program = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  if(!sys) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  if(mtr_obj) {
    struct material* mtr = RDR_GET_OBJECT_DATA(sys, mtr_obj);
    if(!mtr->is_linked) {
      rdr_err = RDR_INVALID_ARGUMENT;
      goto error;
    }
    program = mtr->program;
  } else {
    program = NULL;
  }

  err = sys->rb.bind_program(sys->ctxt, program);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

exit:
  return rdr_err;

error:
  if(sys) {
    err = sys->rb.bind_program(sys->ctxt, NULL);
    assert(err == 0);
  }
  goto exit;
}

enum rdr_error
rdr_get_material_desc
  (struct rdr_system* sys,
   struct rdr_material* mtr_obj,
   struct rdr_material_desc* desc)
{
  struct material* mtr = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!sys || !mtr_obj || !desc) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  mtr = RDR_GET_OBJECT_DATA(sys, mtr_obj);
  desc->attrib_list = mtr->attrib_list;
  desc->uniform_list = mtr->uniform_list;
  desc->nb_attribs = mtr->nb_attribs;
  desc->nb_uniforms = mtr->nb_uniforms;

exit:
  return rdr_err;

error:
  goto exit;
}

enum rdr_error
rdr_is_material_linked
  (struct rdr_system* sys,
   struct rdr_material* mtr_obj,
   bool* is_material_linked)
{
  struct material* mtr = NULL;

  if(!sys || !mtr_obj || !is_material_linked)
    return RDR_INVALID_ARGUMENT;

  mtr = RDR_GET_OBJECT_DATA(sys, mtr_obj);
  *is_material_linked = mtr->is_linked;

  return RDR_NO_ERROR;
}

enum rdr_error
rdr_attach_material_callback
  (struct rdr_system* sys,
   struct rdr_material* mtr_obj,
   const struct rdr_material_callback_desc* cbk_desc,
   struct rdr_material_callback** out_cbk)
{
  struct sl_node* node = NULL;
  struct material* mtr = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !mtr_obj || !cbk_desc|| !out_cbk) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  mtr = RDR_GET_OBJECT_DATA(sys, mtr_obj);
  sl_err = sl_linked_list_add
    (sys->sl_ctxt, mtr->callback_list, cbk_desc, &node);

  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

exit:
  /* the rdr_material_callback is not defined. It simply wrap the sl_node
   * data structure. */
  if(out_cbk)
    *out_cbk = (struct rdr_material_callback*)node;
  return rdr_err;

error:
  if(node) {
    assert(mtr != NULL);
    SL(linked_list_remove(sys->sl_ctxt, mtr->callback_list, node));
    node = NULL;
  }
  goto exit;
}

enum rdr_error
rdr_detach_material_callback
  (struct rdr_system* sys,
   struct rdr_material* mtr_obj,
   struct rdr_material_callback* cbk)
{
  struct sl_node* node = NULL;
  struct material* mtr = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !mtr_obj || !cbk) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  mtr = RDR_GET_OBJECT_DATA(sys, mtr_obj);
  node = (struct sl_node*)cbk;

  sl_err = sl_linked_list_remove(sys->sl_ctxt, mtr->callback_list, node);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

exit:
  return rdr_err;

error:
  goto exit;
}

