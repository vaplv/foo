#include "renderer/regular/rdr_attrib_c.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_material_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_material.h"
#include "renderer/rdr_system.h"
#include "stdlib/sl.h"
#include "stdlib/sl_flat_set.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct rdr_material {
  struct rdr_system* sys;
  struct ref ref;
  struct rb_program* program;
  struct rb_shader* shader_list[RDR_NB_SHADER_USAGES];
  struct rb_attrib** attrib_list;
  struct rb_uniform** uniform_list;
  char* log;
  struct sl_flat_set* callback_set[RDR_NB_MATERIAL_SIGNALS];
  size_t nb_attribs;
  size_t nb_uniforms;
  bool is_linked;
};

/*******************************************************************************
 *
 * Callbacks.
 *
 ******************************************************************************/
struct callback {
  void (*func)(struct rdr_material*, void*);
  void* data;
};

static int
cmp_callbacks(const void* a, const void* b)
{
  struct callback* cbk0 = (struct callback*)a;
  struct callback* cbk1 = (struct callback*)b;
  const uintptr_t p0[2] = {(uintptr_t)cbk0->func, (uintptr_t)cbk0->data};
  const uintptr_t p1[2] = {(uintptr_t)cbk1->func, (uintptr_t)cbk1->data};
  const int inf = (p0[0] < p1[0]) | ((p0[0] == p1[0]) & (p0[1] < p1[1]));
  const int sup = (p0[0] > p1[0]) | ((p0[0] == p1[0]) & (p0[1] > p1[1]));
  return -(inf) | (sup);
}

/*******************************************************************************
 *
 *  Helper functions.
 *
 ******************************************************************************/
static FINLINE enum rb_shader_type 
rdr_shader_to_rb_shader_type(enum rdr_shader_usage usage)
{
  enum rb_shader_type type = RB_VERTEX_SHADER;
  switch(usage) {
    case RDR_VERTEX_SHADER: 
      type = RB_VERTEX_SHADER;
      break;
    case RDR_GEOMETRY_SHADER:
      type = RB_GEOMETRY_SHADER;
      break;
    case RDR_FRAGMENT_SHADER:
      type = RB_FRAGMENT_SHADER;
      break;
    default:
      assert(false);
      break;
  }
  return type;
}

static void
release_shaders
  (struct rdr_system* sys,
   struct rb_program* program,
   struct rb_shader* shader_list[RDR_NB_SHADER_USAGES])
{
  size_t i = 0;

  assert(shader_list);

  for(i = 0; i < RDR_NB_SHADER_USAGES; ++i) {
    struct rb_shader* shader = shader_list[i];

    if(shader !=NULL) {
      int is_attached = 0;
      RBI(&sys->rb, is_shader_attached(shader, &is_attached));
      if(is_attached != 0) {
        RBI(&sys->rb, detach_shader(program, shader));
      }
      RBI(&sys->rb, shader_ref_put(shader));
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
      enum rb_shader_type type = rdr_shader_to_rb_shader_type(i);

      err = sys->rb.create_shader(sys->ctxt, type, src, strlen(src), &shader);
      shader_list[i] = shader;
      if(err != 0) {
        err = sys->rb.get_shader_log(shader, &log);
        if(err == 0) {
          char* tmp_log = NULL;

          assert(log != NULL);
          tmp_log = MEM_ALLOC(sys->allocator, strlen(log) + 1);
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

      err = sys->rb.attach_shader(program, shader);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }
      ++nb_created_shaders;
    }
  }

  err = sys->rb.link_program(program);
  if(err != 0) {
    err = sys->rb.get_program_log(program, &log);
    if(err == 0) {
      char* tmp_log = NULL;

      assert(log != NULL);
      tmp_log = MEM_ALLOC(sys->allocator, strlen(log) + 1);
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
    mtr_attrib_list = MEM_ALLOC
      (sys->allocator, sizeof(struct rb_attrib*) * nb_mtr_attribs);
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
    size_t i = 0;
    for(i = 0; i < nb_mtr_attribs; ++i)
      RBI(&sys->rb, attrib_ref_put(mtr_attrib_list[i]));
    MEM_FREE(sys->allocator, mtr_attrib_list);
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
  assert(sys);

  if(attrib_list) {
    size_t i = 0;
    assert(attrib_list != NULL);

    for(i = 0; i < nb_attribs; ++i) 
      RBI(&sys->rb, attrib_ref_put(attrib_list[i]));
    MEM_FREE(sys->allocator, attrib_list);
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
    uniform_list = MEM_ALLOC
      (sys->allocator, sizeof(struct rb_uniform*) * nb_uniforms);
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
    size_t i = 0;
    for(i = 0; i < nb_uniforms; ++i)
      RBI(&sys->rb, uniform_ref_put(uniform_list[i]));
    MEM_FREE(sys->allocator, uniform_list);
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
  assert(sys);
  if(nb_uniforms) {
    size_t i = 0;
    assert(uniform_list != NULL);

    for(i = 0; i < nb_uniforms; ++i)
      RBI(&sys->rb, uniform_ref_put(uniform_list[i]));

    MEM_FREE(sys->allocator, uniform_list);
  }
}


static void
invoke_callbacks(struct rdr_material* mtr, enum rdr_material_signal sig)
{
  struct callback* buffer = NULL;
  size_t len = 0;
  size_t i = 0;
  assert(mtr);

  SL(flat_set_buffer
    (mtr->callback_set[sig], &len, NULL, NULL, (void**)&buffer));
  for(i = 0; i < len; ++i) {
    const struct callback* clbk = buffer + i;
    clbk->func(mtr, clbk->data);
  }
}

static void
release_material(struct ref* ref)
{
  struct rdr_material* mtr = NULL;
  struct rdr_system* sys = NULL;
  size_t i = 0;
  assert(ref);

  mtr = CONTAINER_OF(ref, struct rdr_material, ref);

  for(i = 0; i < RDR_NB_MATERIAL_SIGNALS; ++i) {
    if(mtr->callback_set[i]) {
      #ifndef NDEBUG
      size_t len = 0;
      SL(flat_set_buffer(mtr->callback_set[i], &len, NULL, NULL, NULL));
      assert(0 == len);
      #endif
      SL(free_flat_set(mtr->callback_set[i]));
    }
  }
  release_attribs(mtr->sys, mtr->nb_attribs, mtr->attrib_list);
  release_uniforms(mtr->sys, mtr->nb_uniforms, mtr->uniform_list);
  if(mtr->log != NULL) {
    MEM_FREE(mtr->sys->allocator, mtr->log);
    mtr->log = NULL;
  }
  if(mtr->program) {
    release_shaders(mtr->sys, mtr->program, mtr->shader_list);
    RBI(&mtr->sys->rb, program_ref_put(mtr->program));
  }
  sys = mtr->sys;
  MEM_FREE(sys->allocator, mtr);
  RDR(system_ref_put(sys));
}

/*******************************************************************************
 *
 * Public render material functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rdr_error
rdr_create_material(struct rdr_system* sys, struct rdr_material** out_mtr)
{
  struct rdr_material* mtr = NULL;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  int err = 0;

  if(!sys || !out_mtr) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  mtr = MEM_CALLOC(sys->allocator, 1, sizeof(struct rdr_material));
  if(!mtr) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  ref_init(&mtr->ref);
  RDR(system_ref_get(sys));
  mtr->sys = sys;

  err = mtr->sys->rb.create_program(mtr->sys->ctxt, &mtr->program);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

  for(i = 0; i < RDR_NB_MATERIAL_SIGNALS; ++i) {
    sl_err = sl_create_flat_set
      (sizeof(struct callback),
       ALIGNOF(struct callback),
       cmp_callbacks,
       mtr->sys->allocator,
       &mtr->callback_set[i]);
    if(sl_err != SL_NO_ERROR) {
      rdr_err = sl_to_rdr_error(sl_err);
      goto error;
    }
  }

exit:
  if(out_mtr)
    *out_mtr = mtr;
  return rdr_err;

error:
  if(mtr) {
    RDR(material_ref_put(mtr));
    mtr = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_material_ref_get(struct rdr_material* mtr)
{
  if(!mtr)
    return RDR_INVALID_ARGUMENT;
  ref_get(&mtr->ref);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_material_ref_put(struct rdr_material* mtr)
{
  if(!mtr)
    return RDR_INVALID_ARGUMENT;
  ref_put(&mtr->ref, release_material);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_get_material_log(struct rdr_material* mtr, const char** out_log)
{
  if(!mtr || !out_log)
    return RDR_INVALID_ARGUMENT;
  *out_log = mtr->log;
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_material_program
  (struct rdr_material* mtr,
   const char* shader_sources[RDR_NB_SHADER_USAGES])
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!mtr || !shader_sources) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  release_attribs(mtr->sys, mtr->nb_attribs, mtr->attrib_list);
  mtr->nb_attribs = 0;
  mtr->attrib_list = NULL;

  release_uniforms(mtr->sys, mtr->nb_uniforms, mtr->uniform_list);
  mtr->nb_uniforms = 0;
  mtr->uniform_list = NULL;

  release_shaders(mtr->sys, mtr->program, mtr->shader_list);

  if(mtr->log) {
    MEM_FREE(mtr->sys->allocator, mtr->log);
    mtr->log = NULL;
  }
  rdr_err = build_program
    (mtr->sys,
     mtr->program,
     shader_sources,
     mtr->shader_list,
     &mtr->log,
     &mtr->is_linked);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  if(mtr->is_linked) {
    rdr_err = get_attribs
      (mtr->sys, mtr->program, &mtr->nb_attribs, &mtr->attrib_list);
    if(rdr_err != RDR_NO_ERROR)
      goto error;

    rdr_err = get_uniforms
      (mtr->sys, mtr->program, &mtr->nb_uniforms, &mtr->uniform_list);
    if(rdr_err != RDR_NO_ERROR)
      goto error;
  }

exit:
  if(mtr)
    invoke_callbacks(mtr, RDR_MATERIAL_SIGNAL_UPDATE_PROGRAM);
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
rdr_bind_material(struct rdr_system* sys, struct rdr_material* mtr)
{
  struct rb_program* program = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  if(!sys || (mtr && mtr->sys != sys)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  if(mtr) {
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
  (struct rdr_material* mtr,
   struct rdr_material_desc* desc)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!mtr || !desc) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

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
rdr_is_material_linked(struct rdr_material* mtr, bool* is_material_linked)
{
  if(!mtr || !is_material_linked)
    return RDR_INVALID_ARGUMENT;
  *is_material_linked = mtr->is_linked;
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_attach_material_callback
  (struct rdr_material* mtr,
   enum rdr_material_signal sig,
   void (*func)(struct rdr_material*, void*),
   void* data)
{
  if(!mtr || !func || sig >= RDR_NB_MATERIAL_SIGNALS)
    return  RDR_INVALID_ARGUMENT;
  SL(flat_set_insert
    (mtr->callback_set[sig], (struct callback[]){{func, data}}, NULL));
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_detach_material_callback
  (struct rdr_material* mtr,
   enum rdr_material_signal sig,
   void (*func)(struct rdr_material*, void*),
   void* data)
{
  if(!mtr || !func || sig >= RDR_NB_MATERIAL_SIGNALS)
    return RDR_INVALID_ARGUMENT;
  SL(flat_set_erase
    (mtr->callback_set[sig], (struct callback[]){{func, data}}, NULL));
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_is_material_callback_attached
  (struct rdr_material* mtr,
   enum rdr_material_signal sig,
   void (*func)(struct rdr_material*, void* data),
   void* data,
   bool* is_attached)
{
  size_t i = 0;
  size_t len = 0;

  if(!mtr || !func || !is_attached || sig >= RDR_NB_MATERIAL_SIGNALS)
    return RDR_INVALID_ARGUMENT;
  SL(flat_set_find
    (mtr->callback_set[sig], (struct callback[]){{func, data}}, &i));
  SL(flat_set_buffer(mtr->callback_set[sig], &len, NULL, NULL, NULL));
  *is_attached = (i != len);
  return RDR_NO_ERROR;
}

