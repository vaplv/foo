#include "maths/simd/aosf44.h"
#include "renderer/regular/rdr_attrib_c.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_model_c.h"
#include "renderer/regular/rdr_model_instance_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_model.h"
#include "renderer/rdr_model_instance.h"
#include "renderer/rdr_system.h"
#include "render_backend/rb_types.h"
#include "stdlib/sl.h"
#include "stdlib/sl_flat_set.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
  NB_MATERIAL_DENSITY = 2,
  NB_FILL_MODES = 2,
  NB_CULL_MODES = 3
};

static const struct rb_blend_desc
material_density_to_blend_desc [NB_MATERIAL_DENSITY] = {
  [RDR_OPAQUE] = {
    .enable = 0,
    .src_blend_RGB = RB_BLEND_ONE,
    .src_blend_Alpha = RB_BLEND_ONE,
    .dst_blend_RGB = RB_BLEND_ZERO,
    .dst_blend_Alpha = RB_BLEND_ZERO,
    .blend_op_RGB = RB_BLEND_OP_ADD,
    .blend_op_Alpha = RB_BLEND_OP_ADD
  },
  [RDR_TRANSLUCENT] = {
    .enable = 1,
    .src_blend_RGB = RB_BLEND_SRC_ALPHA,
    .src_blend_Alpha = RB_BLEND_SRC_ALPHA,
    .dst_blend_RGB = RB_BLEND_ONE_MINUS_SRC_ALPHA,
    .dst_blend_Alpha = RB_BLEND_ONE_MINUS_SRC_ALPHA,
    .blend_op_RGB = RB_BLEND_OP_ADD,
    .blend_op_Alpha = RB_BLEND_OP_ADD
  }
};

static const struct rb_rasterizer_desc
rdr_to_rb_rasterizer[NB_FILL_MODES][NB_CULL_MODES] = {
  [RDR_WIREFRAME] = {
    [RDR_CULL_FRONT] = {
      .fill_mode = RB_FILL_WIREFRAME,
      .cull_mode = RB_CULL_FRONT,
      .front_facing = RB_ORIENTATION_CCW
    },
    [RDR_CULL_BACK] = {
      .fill_mode = RB_FILL_WIREFRAME,
      .cull_mode = RB_CULL_BACK,
      .front_facing = RB_ORIENTATION_CCW
    },
    [RDR_CULL_NONE] = {
      .fill_mode = RB_FILL_WIREFRAME,
      .cull_mode = RB_CULL_NONE,
      .front_facing = RB_ORIENTATION_CCW
    }
  },
  [RDR_SOLID] = {
    [RDR_CULL_FRONT] = {
      .fill_mode = RB_FILL_SOLID,
      .cull_mode = RB_CULL_FRONT,
      .front_facing = RB_ORIENTATION_CCW
    },
    [RDR_CULL_BACK] = {
      .fill_mode = RB_FILL_SOLID,
      .cull_mode = RB_CULL_BACK,
      .front_facing = RB_ORIENTATION_CCW
    },
    [RDR_CULL_NONE] = {
      .fill_mode = RB_FILL_SOLID,
      .cull_mode = RB_CULL_NONE,
      .front_facing = RB_ORIENTATION_CCW
    }
  }
};

struct rdr_model_instance {
  /* Column major transform matrix of the instance. */
  struct aosf44 transform;
  /* The model from which the instance is created. */
  struct rdr_model* model;
  /* List of callbacks to call when the instance change. */
  struct sl_flat_set* callback_set;
  /* Data of the model instance. */
  void* uniform_buffer;
  void* attrib_buffer;
  /* */
  enum rdr_material_density material_density;
  struct rdr_rasterizer_desc rasterizer_desc;
  struct ref ref;
  struct rdr_system* sys;
};

/*******************************************************************************
 *
 * Callbacks.
 *
 ******************************************************************************/
struct callback {
  void (*func)(struct rdr_model_instance*, void*);
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
 * Helper functions.
 *
 ******************************************************************************/
static enum rdr_error
setup_uniform_data_list
  (struct rdr_system* sys,
   size_t nb_uniforms,
   const struct rdr_model_uniform* model_uniform_list,
   void* uniform_buffer,
   size_t* out_nb_uniform_data,
   struct rdr_model_instance_data* uniform_data_list)
{
  void* buffer = uniform_buffer;
  size_t nb_uniform_data = 0;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(sys
      && (!nb_uniforms || model_uniform_list)
      && (!nb_uniforms || uniform_buffer)
      && out_nb_uniform_data);

  for(i = 0; i < nb_uniforms; ++i) {
    struct rb_uniform_desc uniform_desc;
    int err = 0;

    err = sys->rb.get_uniform_desc
      (model_uniform_list[i].uniform, &uniform_desc);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }

    if(model_uniform_list[i].usage == RDR_REGULAR_UNIFORM) {
      if(uniform_data_list != NULL) {
        uniform_data_list[nb_uniform_data].name = uniform_desc.name;
        uniform_data_list[nb_uniform_data].data = buffer;
        uniform_data_list[nb_uniform_data].type =
          rb_to_rdr_type(uniform_desc.type);
      }
      ++nb_uniform_data;
    }
    buffer = (void*)((uintptr_t)buffer + sizeof_rb_type(uniform_desc.type));
  }

exit:
  *out_nb_uniform_data = nb_uniform_data;
  return rdr_err;

error:
  nb_uniform_data = 0;
  goto exit;
}

static enum rdr_error
setup_attrib_data_list
  (struct rdr_system* sys,
   size_t nb_attribs,
   struct rb_attrib* model_attrib_list[],
   void* attrib_buffer,
   size_t* out_nb_attrib_data,
   struct rdr_model_instance_data* attrib_data_list)
{
  void* buffer = attrib_buffer;
  size_t nb_attrib_data = nb_attribs;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(sys
      && (!nb_attribs || model_attrib_list)
      && (!nb_attribs || attrib_buffer)
      && out_nb_attrib_data);

  if(attrib_data_list != NULL) {
    size_t i = 0;

    for(i = 0; i < nb_attribs; ++i) {
      struct rb_attrib_desc attrib_desc;
      int err = 0;

      err = sys->rb.get_attrib_desc(model_attrib_list[i], &attrib_desc);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }

      attrib_data_list[i].type = rb_to_rdr_type(attrib_desc.type);
      attrib_data_list[i].name = attrib_desc.name;
      attrib_data_list[i].data = buffer;

      buffer = (void*)((uintptr_t)buffer + sizeof_rb_type(attrib_desc.type));
    }
  }

exit:
  *out_nb_attrib_data = nb_attrib_data;
  return rdr_err;

error:
  nb_attrib_data = 0;
  goto exit;
}

static enum rdr_error
dispatch_uniform_data
  (struct rdr_system* sys,
   size_t nb_uniforms,
   struct rdr_model_uniform* model_uniform_list,
   void* uniform_data_list,
   const struct aosf44* transform,
   const struct aosf44* view_matrix,
   const struct aosf44* proj_matrix)
{
  struct aosf44 tmp_mat44;
  ALIGN(16) float mat[16];
  void* data;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  memset(mat, 0, sizeof(mat));

  assert(sys
      && (!nb_uniforms || (uniform_data_list && model_uniform_list))
      && transform
      && view_matrix
      && proj_matrix);

  data = (float*)uniform_data_list;
  for(i = 0; i < nb_uniforms; ++i) {
    struct rb_uniform_desc uniform_desc;
    int err = 0;

    err = sys->rb.get_uniform_desc
      (model_uniform_list[i].uniform, &uniform_desc);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }

    /* TODO: cache the matrix transformation. */
    switch(model_uniform_list[i].usage) {
      case RDR_MODELVIEW_UNIFORM:
        aosf44_mulf44(&tmp_mat44, view_matrix, transform);
        aosf44_store(mat, &tmp_mat44);
        memcpy(data, mat, sizeof(mat));
        break;
      case RDR_PROJECTION_UNIFORM:
        aosf44_store(mat, proj_matrix);
        memcpy(data, mat, sizeof(mat));
        break;
      case RDR_VIEWPROJ_UNIFORM:
        aosf44_mulf44(&tmp_mat44, view_matrix, transform);
        aosf44_mulf44(&tmp_mat44, proj_matrix, &tmp_mat44);
        aosf44_store(mat, &tmp_mat44);
        memcpy(data, mat, sizeof(mat));
        break;
      case RDR_MODELVIEW_INVTRANS_UNIFORM:
        aosf44_mulf44(&tmp_mat44, view_matrix, transform);
        aosf44_invtrans(&tmp_mat44, &tmp_mat44);
        aosf44_store(mat, &tmp_mat44);
        memcpy(data, mat, sizeof(mat));
        break;
      case RDR_REGULAR_UNIFORM:
        /* nothing */
        break;
      default:
        assert(false);
        break;
    }

    err = sys->rb.uniform_data
      (model_uniform_list[i].uniform, 1, data);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }
    data = (void*)((uintptr_t)data + sizeof_rb_type(uniform_desc.type));
  }

exit:
  return rdr_err;

error:
  goto exit;
}

static enum rdr_error
dispatch_attrib_data
  (struct rdr_system* sys,
   size_t nb_attribs,
   struct rb_attrib* model_attrib_list[],
   const void* attrib_data_list)
{
  const void* data = NULL;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(sys && (!nb_attribs || (attrib_data_list && model_attrib_list)));

  data = attrib_data_list;
  for(i = 0; i < nb_attribs; ++i) {
    struct rb_attrib_desc attrib_desc;
    int err = 0;

    err = sys->rb.get_attrib_desc(model_attrib_list[i], &attrib_desc);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }

    err = sys->rb.attrib_data(model_attrib_list[i], data);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }
    data = (void*)((uintptr_t)data + sizeof_rb_type(attrib_desc.type));
  }

exit:
  return rdr_err;

error:
  goto exit;
}

static void
invoke_callbacks(struct rdr_model_instance* instance)
{
  struct callback* buffer = NULL;
  size_t len = 0;
  size_t i = 0;
  assert(instance);

  SL(flat_set_buffer
    (instance->callback_set, &len, NULL, NULL, (void**)&buffer));
  for(i = 0; i < len; ++i) {
    const struct callback* clbk = buffer + i;
    clbk->func(instance, clbk->data);
  }
}

static enum rdr_error
setup_model_instance_buffers
  (struct rdr_model_instance* instance,
   const struct rdr_model_desc* model_desc)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(model_desc
      && instance
      && !instance->uniform_buffer
      && !instance->attrib_buffer);

  if(model_desc->sizeof_uniform_data) {
    instance->uniform_buffer = MEM_CALLOC
      (instance->sys->allocator, 1, model_desc->sizeof_uniform_data);
    if(!instance->uniform_buffer) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }
  }
  if(model_desc->sizeof_attrib_data) {
    instance->attrib_buffer = MEM_CALLOC
      (instance->sys->allocator, 1, model_desc->sizeof_attrib_data);
    if(!instance->attrib_buffer) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }
  }

exit:
  return rdr_err;

error:
  if(instance->uniform_buffer)
    MEM_FREE(instance->sys->allocator, instance->uniform_buffer);
  if(instance->attrib_buffer)
    MEM_FREE(instance->sys->allocator, instance->attrib_buffer);

  goto exit;
}

static void
model_callback_func(struct rdr_model* model, void* data)
{
  struct rdr_model_desc model_desc;
  struct rdr_model_instance* instance = data;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  assert(instance);

  RDR(get_model_desc(model, &model_desc));
  if(instance->uniform_buffer) {
    MEM_FREE(instance->sys->allocator, instance->uniform_buffer);
    instance->uniform_buffer = NULL;
  }
  if(instance->attrib_buffer) {
    MEM_FREE(instance->sys->allocator, instance->attrib_buffer);
    instance->attrib_buffer = NULL;
  }
  rdr_err = setup_model_instance_buffers(instance, &model_desc);
  assert(RDR_NO_ERROR == rdr_err);
  invoke_callbacks(instance);
}


/* return true if the box is infinite. */
static bool
get_model_instance_obb
  (const struct rdr_model_instance* instance,
   vf4_t* restrict position,
   vf4_t* restrict extend_x,
   vf4_t* restrict extend_y,
   vf4_t* restrict extend_z)
{
  struct rdr_mesh* mesh = NULL;
  float min_bound[3] = { 0.f, 0.f, 0.f };
  float max_bound[3] = { 0.f, 0.f, 0.f };
  bool is_infinite = false;
  assert(instance && position && extend_x && extend_y && extend_z);

  RDR(get_model_mesh(instance->model, &mesh));
  RDR(get_mesh_aabb(mesh, min_bound, max_bound));

  is_infinite =
      (min_bound[0] == -FLT_MAX)
    | (min_bound[1] == -FLT_MAX)
    | (min_bound[2] == -FLT_MAX)
    | (max_bound[0] == FLT_MAX)
    | (max_bound[1] == FLT_MAX)
    | (max_bound[2] == FLT_MAX);

  if(is_infinite) {
    *position = vf4_zero();
    *extend_x = *extend_y = *extend_z = vf4_set1(FLT_MAX);
  } else {
    const struct aosf33 f33 = {
      .c0 = instance->transform.c0, 
      .c1 = instance->transform.c1, 
      .c2 = instance->transform.c2
    };
    const vf4_t vmin = vf4_set(min_bound[0], min_bound[1], min_bound[2], 1.f);
    const vf4_t vmax = vf4_set(max_bound[0], max_bound[1], max_bound[2], 1.f);
    const vf4_t vhmin = vf4_mul(vmin, vf4_set(0.5f, 0.5f, 0.5f, 1.f));
    const vf4_t vhmax = vf4_mul(vmax, vf4_set(0.5f, 0.5f, 0.5f, 1.f));
    const vf4_t vpos = vf4_add(vhmax, vhmin);
    const vf4_t vext = vf4_sub(vmax, vpos);
    const vf4_t vextx = vf4_and(vext, vf4_mask(true, false, false, true));
    const vf4_t vexty = vf4_and(vext, vf4_mask(false, true, false, true));
    const vf4_t vextz = vf4_and(vext, vf4_mask(false, false, true, true));

    *position = vf4_add(aosf33_mulf3(&f33, vpos), instance->transform.c3);
    *extend_x = aosf33_mulf3(&f33, vextx);
    *extend_y = aosf33_mulf3(&f33, vexty);
    *extend_z = aosf33_mulf3(&f33, vextz);
  }
  return is_infinite;
}

static void
release_model_instance(struct ref* ref)
{
  struct rdr_model_instance* instance = NULL;
  struct rdr_system* sys = NULL;
  bool b = false;
  assert(ref);

  instance = CONTAINER_OF(ref, struct rdr_model_instance, ref);

  if(instance->callback_set) {
    #ifndef NDEBUG
    size_t len = 0;
    SL(flat_set_buffer(instance->callback_set, &len, NULL, NULL, NULL));
    assert(len == 0);
    #endif
    SL(free_flat_set(instance->callback_set));
  }
  RDR(is_model_callback_attached
    (instance->model,
     RDR_MODEL_SIGNAL_UPDATE_DATA,
     model_callback_func,
     instance,
     &b));
  if(b) {
    RDR(detach_model_callback
      (instance->model,
       RDR_MODEL_SIGNAL_UPDATE_DATA,
       model_callback_func,
       instance));
  }
  if(instance->uniform_buffer)
    MEM_FREE(instance->sys->allocator, instance->uniform_buffer);
  if(instance->attrib_buffer)
    MEM_FREE(instance->sys->allocator, instance->attrib_buffer);

  RDR(model_ref_put(instance->model));
  sys = instance->sys;
  MEM_FREE(instance->sys->allocator, instance);
  RDR(system_ref_put(sys));
}

/*******************************************************************************
 *
 * Implementation of the render model instance functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rdr_error
rdr_create_model_instance
  (struct rdr_system* sys,
   struct rdr_model* model,
   struct rdr_model_instance** out_instance)
{
  const struct rdr_rasterizer_desc default_raster = {
    .cull_mode = RDR_CULL_BACK,
    .fill_mode = RDR_SOLID
  };
  const float identity[16] = {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
  };
  struct rdr_model_desc model_desc;
  struct rdr_model_instance* instance = NULL;
  enum sl_error sl_err = SL_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!sys || !model || !out_instance) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  instance = MEM_ALIGNED_ALLOC
    (sys->allocator, sizeof(struct rdr_model_instance), 16);
  if(!instance) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  memset(instance, 0, sizeof(struct rdr_model_instance));
  ref_init(&instance->ref);
  RDR(model_ref_get(model));
  instance->model = model;
  RDR(system_ref_get(sys));
  instance->sys = sys;

  sl_err = sl_create_flat_set
    (sizeof(struct callback),
     ALIGNOF(struct callback),
     cmp_callbacks,
     sys->allocator,
     &instance->callback_set);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }
  #define CALL(func) \
    do { \
      if(RDR_NO_ERROR != (rdr_err = func)) \
        goto error; \
    } while(0)
  CALL(rdr_attach_model_callback
    (model, RDR_MODEL_SIGNAL_UPDATE_DATA, &model_callback_func, instance));
  CALL(rdr_get_model_desc(model, &model_desc));
  CALL(setup_model_instance_buffers(instance, &model_desc));
  CALL(rdr_model_instance_transform(instance, identity));
  CALL(rdr_model_instance_material_density(instance, RDR_OPAQUE));
  CALL(rdr_model_instance_rasterizer(instance, &default_raster));
  #undef CALL

exit:
  if(out_instance)
    *out_instance = instance;
  return rdr_err;

error:
  if(instance) {
    RDR(model_instance_ref_put(instance));
    instance = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_model_instance_ref_get(struct rdr_model_instance* instance)
{
  if(!instance)
    return RDR_INVALID_ARGUMENT;
  ref_get(&instance->ref);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_model_instance_ref_put(struct rdr_model_instance* instance)
{
  if(!instance)
    return RDR_INVALID_ARGUMENT;
  ref_put(&instance->ref, release_model_instance);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_get_model_instance_uniforms
  (struct rdr_model_instance* instance,
   size_t* out_nb_uniforms,
   struct rdr_model_instance_data* uniform_data_list)
{
  struct rdr_model_desc model_desc;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  memset(&model_desc, 0, sizeof(struct rdr_model_desc));

  if(!instance || !out_nb_uniforms) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  rdr_err = rdr_get_model_desc(instance->model, &model_desc);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = setup_uniform_data_list
    (instance->sys,
     model_desc.nb_uniforms,
     model_desc.uniform_list,
     instance->uniform_buffer,
     out_nb_uniforms,
     uniform_data_list);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  return rdr_err;
error:
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_get_model_instance_attribs
  (struct rdr_model_instance* instance,
   size_t* out_nb_attribs,
   struct rdr_model_instance_data* attrib_data_list)
{
  struct rdr_model_desc model_desc;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  memset(&model_desc, 0, sizeof(struct rdr_model_desc));

  if(!instance || !out_nb_attribs) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  rdr_err = rdr_get_model_desc(instance->model, &model_desc);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = setup_attrib_data_list
    (instance->sys,
     model_desc.nb_attribs,
     model_desc.attrib_list,
     instance->attrib_buffer,
     out_nb_attribs,
     attrib_data_list);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  return rdr_err;
error:
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_model_instance_transform
  (struct rdr_model_instance* instance,
   const float mat[16])
{
  if(!instance || !mat)
    return RDR_INVALID_ARGUMENT;
  instance->transform.c0 = vf4_set(mat[0], mat[1], mat[2], mat[3]);
  instance->transform.c1 = vf4_set(mat[4], mat[5], mat[6], mat[7]);
  instance->transform.c2 = vf4_set(mat[8], mat[9], mat[10], mat[11]);
  instance->transform.c3 = vf4_set(mat[12], mat[13], mat[14], mat[15]);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_get_model_instance_transform
  (const struct rdr_model_instance* instance,
   float transform[16])
{
  ALIGN(16) float mat[16];

  if(!instance || !transform)
    return RDR_INVALID_ARGUMENT;
  aosf44_store(mat, &instance->transform);
  transform = memcpy(transform, mat, sizeof(float) * 16);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_translate_model_instances
  (struct rdr_model_instance* instance_list[],
   size_t nb_instances,
   bool local_translation,
   const float trans[3])
{
  size_t i = 0;

  if((nb_instances && !instance_list) || !trans)
    return RDR_INVALID_ARGUMENT;
  if((!trans[0]) & (!trans[1]) & (!trans[2]))
    return RDR_NO_ERROR;

  if(local_translation) {
    const vf4_t vec = vf4_set(trans[0], trans[1], trans[2], 1.f);
    for(i = 0; i < nb_instances; ++i) {
      struct rdr_model_instance* instance = instance_list[i];
      instance->transform.c3 = aosf44_mulf4(&instance->transform, vec);
    }
  } else {
    const vf4_t vec = vf4_set(trans[0], trans[1], trans[2], 0.f);
    for(i = 0; i < nb_instances; ++i) {
      struct rdr_model_instance* instance = instance_list[i];
      instance->transform.c3 = vf4_add(instance->transform.c3, vec);
    }
  }
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_rotate_model_instances
  (struct rdr_model_instance* instance_list[],
   size_t nb_instances,
   bool local_rotation,
   const float rotation[3])
{
  struct aosf33 f33;
  size_t i = 0;
  enum { PITCH, YAW, ROLL };

  if((nb_instances && !instance_list) || !rotation)
    return RDR_INVALID_ARGUMENT;
  if((!rotation[PITCH]) & (!rotation[YAW]) & (!rotation[ROLL]))
     return RDR_NO_ERROR;

  aosf33_rotation(&f33, rotation[PITCH], rotation[YAW], rotation[ROLL]);

  if(local_rotation) {
    for(i = 0; i < nb_instances; ++i) {
      struct rdr_model_instance* instance = instance_list[i];
      struct aosf33 res;
      const struct aosf33 tmp = {
        .c0 = instance->transform.c0,
        .c1 = instance->transform.c1,
        .c2 = instance->transform.c2
      };
      aosf33_mulf33(&res, &tmp, &f33);
      instance->transform.c0 = res.c0;
      instance->transform.c1 = res.c1;
      instance->transform.c2 = res.c2;
    }
  } else {
    const struct aosf44 f44 = {
      .c0 = f33.c0,
      .c1 = f33.c1,
      .c2 = f33.c2,
      .c3 = vf4_set(0.f, 0.f, 0.f, 1.f)
    };
    for(i = 0; i < nb_instances; ++i) {
      struct rdr_model_instance* instance = instance_list[i];
      aosf44_mulf44(&instance->transform, &f44, &instance->transform);
    }
  }
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_scale_model_instances
  (struct rdr_model_instance* instance_list[],
   size_t nb_instances,
   bool local_scale,
   const float scale[3])
{
  struct aosf33 f33;
  size_t i = 0;

  if((nb_instances && !instance_list) || !scale)
    return RDR_INVALID_ARGUMENT;
  if((scale[0] == 1.f) & (scale[1] == 1.f) & (scale[2] == 1.f))
    return RDR_NO_ERROR;

  f33.c0 = vf4_set(scale[0], 0.f, 0.f, 0.f);
  f33.c1 = vf4_set(0.f, scale[1], 0.f, 0.f);
  f33.c2 = vf4_set(0.f, 0.f, scale[2], 0.f);

  if(local_scale) {
    for(i = 0; i < nb_instances; ++i) {
      struct rdr_model_instance* instance = instance_list[i];
      struct aosf33 tmp = {
        .c0 = instance->transform.c0,
        .c1 = instance->transform.c1,
        .c2 = instance->transform.c2
      };
      aosf33_mulf33(&tmp, &tmp, &f33);
      instance->transform.c0 = tmp.c0;
      instance->transform.c1 = tmp.c1;
      instance->transform.c2 = tmp.c2;
    }
  } else {
    const struct aosf44 f44 = {
      .c0 = f33.c0,
      .c1 = f33.c1,
      .c2 = f33.c2,
      .c3 = vf4_set(0.f, 0.f, 0.f, 1.f)
    };
    for(i = 0; i < nb_instances; ++i) {
      struct rdr_model_instance* instance = instance_list[i];
      aosf44_mulf44(&instance->transform, &f44, &instance->transform);
    }
  }
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_move_model_instances
  (struct rdr_model_instance* instance_list[],
   size_t nb_instances,
   const float pos[3])
{
  size_t i = 0;
  if((nb_instances && !instance_list) || !pos)
    return RDR_INVALID_ARGUMENT;

  for(i = 0; i < nb_instances; ++i) {
    struct rdr_model_instance* instance = instance_list[i];
    instance->transform.c3 = vf4_set(pos[0], pos[1], pos[2], 1.f);
  }
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_transform_model_instances
  (struct rdr_model_instance* instance_list[],
   size_t nb_instances,
   bool local_transformation,
   const struct aosf44* transform)
{
  size_t i = 0;

  if((nb_instances && !instance_list) || !transform)
    return RDR_INVALID_ARGUMENT;

  if(local_transformation) {
    for(i = 0; i < nb_instances; ++i) {
      struct rdr_model_instance* instance = instance_list[i];
      aosf44_mulf44(&instance->transform, &instance->transform, transform);
    }
  } else {
    for(i = 0; i < nb_instances; ++i) {
      struct rdr_model_instance* instance = instance_list[i];
      aosf44_mulf44(&instance->transform, transform, &instance->transform);
    }
  }
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_model_instance_material_density
  (struct rdr_model_instance* instance,
   enum rdr_material_density density)
{
  if(!instance)
    return RDR_INVALID_ARGUMENT;
  instance->material_density = density;
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_get_model_instance_material_density
  (const struct rdr_model_instance* instance,
   enum rdr_material_density* out_density)
{
  if(!instance || !out_density)
    return RDR_INVALID_ARGUMENT;
  *out_density = instance->material_density;
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_model_instance_rasterizer
  (struct rdr_model_instance* instance,
   const struct rdr_rasterizer_desc* rasterizer_desc)
{
  if(!instance || !rasterizer_desc)
    return RDR_INVALID_ARGUMENT;
  memcpy
    (&instance->rasterizer_desc,
     rasterizer_desc,
     sizeof(struct rdr_rasterizer_desc));
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_get_model_instance_rasterizer
  (const struct rdr_model_instance* instance,
   struct rdr_rasterizer_desc* out_rasterizer_desc)
{
  if(!instance || !out_rasterizer_desc)
    return RDR_INVALID_ARGUMENT;
  memcpy
    (out_rasterizer_desc,
     &instance->rasterizer_desc,
     sizeof(struct rdr_rasterizer_desc));
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_get_model_instance_model
  (struct rdr_model_instance* instance,
   struct rdr_model** mdl)
{
  if(UNLIKELY(!instance || !mdl))
    return RDR_INVALID_ARGUMENT;
  *mdl = instance->model;
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_get_model_instance_aabb
  (const struct rdr_model_instance* instance,
   float min_bound[3],
   float max_bound[3])
{
  vf4_t vpos, vx, vy, vz;
  bool infinite_box = false;

  if(UNLIKELY(!instance || !min_bound || !max_bound))
    return RDR_INVALID_ARGUMENT;

  infinite_box = get_model_instance_obb(instance, &vpos, &vx, &vy, &vz);
  if(infinite_box) {
    min_bound[0] = min_bound[1] = min_bound[2] = -FLT_MAX;
    max_bound[0] = max_bound[1] = max_bound[2] = FLT_MAX;
  } else {
    vf4_t vmin, vmax;
    ALIGN(16) float tmp[4];

    vmin = vmax = vf4_add(vf4_add(vf4_add(vpos, vx), vy), vz);

    #define NEG sub
    #define POS add
    #define UPDATE_BOUND(x, y, z) \
      do { \
        const vf4_t vtmp = \
          CONCAT(vf4_, z)(CONCAT(vf4_, y)(CONCAT(vf4_, x)(vpos, vx), vy), vz); \
        vmin = vf4_min(vmin, vtmp); \
        vmax = vf4_max(vmax, vtmp); \
      } while(0)
    UPDATE_BOUND(NEG, POS, POS);
    UPDATE_BOUND(POS, NEG, POS);
    UPDATE_BOUND(NEG, NEG, POS);
    UPDATE_BOUND(POS, POS, NEG);
    UPDATE_BOUND(NEG, POS, NEG);
    UPDATE_BOUND(POS, NEG, NEG);
    UPDATE_BOUND(NEG, NEG, NEG);
    #undef NEG
    #undef POS
    #undef UPDATE_BOUND

    vf4_store(tmp, vmin);
    min_bound[0] = tmp[0];
    min_bound[1] = tmp[1];
    min_bound[2] = tmp[2];
    vf4_store(tmp, vmax);
    max_bound[0] = tmp[0];
    max_bound[1] = tmp[1];
    max_bound[2] = tmp[2];
  }
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_get_model_instance_obb
  (const struct rdr_model_instance* instance,
   float position[3],
   float extend_x[3],
   float extend_y[3],
   float extend_z[3])
{
  vf4_t vpos, vx, vy, vz;
  ALIGN(16) float tmp[4];

  if(UNLIKELY(!instance || !position || !extend_x || !extend_y || !extend_z))
    return RDR_INVALID_ARGUMENT;

  get_model_instance_obb(instance, &vpos, &vx, &vy, &vz);
  vf4_store(tmp, vpos);
  position[0] = tmp[0];
  position[1] = tmp[1];
  position[2] = tmp[2];
  vf4_store(tmp, vx);
  extend_x[0] = tmp[0];
  extend_x[1] = tmp[1];
  extend_x[2] = tmp[2];
  vf4_store(tmp, vy);
  extend_y[0] = tmp[0];
  extend_y[1] = tmp[1];
  extend_y[2] = tmp[2];
  vf4_store(tmp, vz);
  extend_z[0] = tmp[0];
  extend_z[1] = tmp[1];
  extend_z[2] = tmp[2];
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_attach_model_instance_callback
  (struct rdr_model_instance* instance,
   void (*func)(struct rdr_model_instance*, void*),
   void* data)
{
  if(!instance || !func)
    return  RDR_INVALID_ARGUMENT;
  SL(flat_set_insert
    (instance->callback_set, (struct callback[]){{func, data}}, NULL));
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_detach_model_instance_callback
  (struct rdr_model_instance* instance,
   void (*func)(struct rdr_model_instance*, void*),
   void* data)
{
  bool b = false;

  if(!instance || !func)
    return RDR_INVALID_ARGUMENT;

  RDR(is_model_instance_callback_attached(instance, func, data, &b));
  if(!b)
    return RDR_INVALID_ARGUMENT;
  SL(flat_set_erase
    (instance->callback_set, (struct callback[]){{func, data}}, NULL));
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_is_model_instance_callback_attached
  (struct rdr_model_instance* instance,
   void (*func)(struct rdr_model_instance*, void* data),
   void* data,
   bool* is_attached)
{
  size_t i = 0;
  size_t len = 0;

  if(!instance || !func || !is_attached)
    return RDR_INVALID_ARGUMENT;
  SL(flat_set_find
    (instance->callback_set, (struct callback[]){{func, data}}, &i));
  SL(flat_set_buffer(instance->callback_set, &len, NULL, NULL, NULL));
  *is_attached = (i != len);
  return RDR_NO_ERROR;
}

/*******************************************************************************
 *
 * Private functions.
 *
 ******************************************************************************/
enum rdr_error
rdr_draw_instances
  (struct rdr_system* sys,
   const struct aosf44* view_matrix,
   const struct aosf44* proj_matrix,
   size_t nb_instances,
   struct rdr_model_instance** instance_list)
{
  struct rdr_model_desc bound_mdl_desc;
  struct rdr_model* bound_mdl = NULL;
  size_t nb_bound_indices = 0;
  size_t i = 0;
  enum rdr_material_density used_density = NB_MATERIAL_DENSITY;
  enum rdr_fill_mode used_fill_mode = NB_FILL_MODES;
  enum rdr_cull_mode used_cull_mode = NB_CULL_MODES;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  if(!sys || !view_matrix || !proj_matrix || (nb_instances && !instance_list)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  for(i = 0; i < nb_instances; ++i) {
    struct rdr_model_instance* instance = instance_list[i];
    if(instance == NULL) {
      rdr_err = RDR_INVALID_ARGUMENT;
      goto error;
    }
    if(instance->model != bound_mdl) {
      rdr_err = rdr_bind_model(sys, instance->model, &nb_bound_indices);
      if(rdr_err != RDR_NO_ERROR)
        goto error;
      bound_mdl = instance->model;

      rdr_err = rdr_get_model_desc(bound_mdl, &bound_mdl_desc);
      if(rdr_err != RDR_NO_ERROR)
        goto error;
    }

    rdr_err = dispatch_uniform_data
      (sys,
       bound_mdl_desc.nb_uniforms,
       bound_mdl_desc.uniform_list,
       instance->uniform_buffer,
       &instance->transform,
       view_matrix,
       proj_matrix);
    if(rdr_err != RDR_NO_ERROR)
      goto error;

    rdr_err = dispatch_attrib_data
      (sys,
       bound_mdl_desc.nb_attribs,
       bound_mdl_desc.attrib_list,
       instance->attrib_buffer);
    if(rdr_err != RDR_NO_ERROR)
      goto error;

    if(used_density != instance->material_density) {
      used_density = instance->material_density;
      err = sys->rb.blend
        (sys->ctxt, &material_density_to_blend_desc[used_density]);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }
    }

    if(used_fill_mode != instance->rasterizer_desc.fill_mode
    || used_cull_mode != instance->rasterizer_desc.cull_mode) {
      used_fill_mode = instance->rasterizer_desc.fill_mode;
      used_cull_mode = instance->rasterizer_desc.cull_mode;
      err = sys->rb.rasterizer
        (sys->ctxt, &rdr_to_rb_rasterizer[used_fill_mode][used_cull_mode]);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }
    }

    err = sys->rb.draw_indexed(sys->ctxt, RB_TRIANGLE_LIST, nb_bound_indices);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }
  }

exit:
  if(sys)
    RDR(bind_model(sys, NULL, NULL));
  return rdr_err;

error:
  goto exit;
}

