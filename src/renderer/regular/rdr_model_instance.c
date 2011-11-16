#include "maths/simd/aosf44.h"
#include "renderer/regular/rdr_attrib_c.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_model_c.h"
#include "renderer/regular/rdr_model_instance_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr_model.h"
#include "renderer/rdr_model_instance.h"
#include "render_backend/rb_types.h"
#include "stdlib/sl_linked_list.h"
#include "sys/sys.h"
#include <assert.h>
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

struct model_instance {
  /* The model from which the instance is created. */
  struct rdr_model* model;
  /*  Invoked when the model has updated is descriptor data. */
  struct rdr_model_callback* model_callback;
  /* Chain list of callbacks to call when the instance change. */
  struct sl_linked_list* callback_list;
  /* Data of the model instance. */
  void* uniform_buffer;
  void* attrib_buffer;
  /* Column major transform matrix of the instance. */
  struct aosf44 transform;
  /* */
  enum rdr_material_density material_density;
  struct rdr_rasterizer_desc rasterizer_desc;
};

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
      (sys->ctxt, model_uniform_list[i].uniform, &uniform_desc);
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
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(sys
      && (!nb_attribs || model_attrib_list)
      && (!nb_attribs || attrib_buffer)
      && out_nb_attrib_data);

  if(attrib_data_list != NULL) {
    for(i = 0; i < nb_attribs; ++i) {
      struct rb_attrib_desc attrib_desc;
      int err = 0;

      err = sys->rb.get_attrib_desc
        (sys->ctxt, model_attrib_list[i], &attrib_desc);
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
      (sys->ctxt, model_uniform_list[i].uniform, &uniform_desc);
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
      (sys->ctxt, model_uniform_list[i].uniform, 1, data);
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

    err = sys->rb.get_attrib_desc
      (sys->ctxt, model_attrib_list[i], &attrib_desc);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }

    err = sys->rb.attrib_data(sys->ctxt, model_attrib_list[i], data);
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

static enum rdr_error
invoke_callbacks
  (struct rdr_system* sys, 
   struct rdr_model_instance* instance_obj)
{
  struct sl_node* node = NULL;
  struct model_instance* instance = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(sys&& instance_obj);

  instance = RDR_GET_OBJECT_DATA(sys, instance_obj);

  for(SL(linked_list_head(instance->callback_list, &node));
      node != NULL;
      SL(next_node(node, &node))) {
    struct rdr_model_instance_callback_desc* desc = NULL;
    enum rdr_error last_err = RDR_NO_ERROR;

    SL(node_data(node, (void**)&desc));

    last_err = desc->func(sys, instance_obj, desc->data);
    if(last_err != RDR_NO_ERROR)
      rdr_err = last_err;
  }
  return rdr_err;
}

static enum rdr_error
setup_model_instance_buffers
  (struct rdr_system* sys UNUSED,
   struct model_instance* instance,
   const struct rdr_model_desc* model_desc)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(sys
      && model_desc
      && instance
      && !instance->uniform_buffer
      && !instance->attrib_buffer);

  if(model_desc->sizeof_uniform_data) {
    instance->uniform_buffer = calloc(1, model_desc->sizeof_uniform_data);
    if(!instance->uniform_buffer) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }
  }

  if(model_desc->sizeof_attrib_data) {
    instance->attrib_buffer = calloc(1, model_desc->sizeof_attrib_data);
    if(!instance->attrib_buffer) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }
  }

error:
  return rdr_err;

exit:
  if(instance->uniform_buffer)
    free(instance->uniform_buffer);

  if(instance->attrib_buffer)
    free(instance->attrib_buffer);

  goto exit;
}

static enum rdr_error
model_callback_func
  (struct rdr_system* sys,
   struct rdr_model* model,
   void* data)
{
  struct rdr_model_desc model_desc;
  struct rdr_model_instance* instance_obj = data;
  struct model_instance* instance = NULL;;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum rdr_error tmp_err = RDR_NO_ERROR;

  assert(instance_obj);

  instance = RDR_GET_OBJECT_DATA(sys, instance_obj);

  rdr_err = rdr_get_model_desc(sys, model, &model_desc);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  if(instance->uniform_buffer) {
    free(instance->uniform_buffer);
    instance->uniform_buffer = NULL;
  }

  if(instance->attrib_buffer) {
    free(instance->attrib_buffer);
    instance->attrib_buffer = NULL;
  }

  rdr_err = setup_model_instance_buffers(sys, instance, &model_desc);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  tmp_err = invoke_callbacks(sys, instance_obj);
  if(tmp_err != RDR_NO_ERROR)
    rdr_err = tmp_err;

  return rdr_err;

error:
  goto exit;
}

static void
free_model_instance(struct rdr_system* sys, void* data)
{
  struct model_instance* instance = data;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(sys && instance && instance->model);

  if(instance->callback_list) {
    struct sl_node* node = NULL;

    SL(linked_list_head(instance->callback_list, &node));
    assert(node == NULL);
    SL(free_linked_list(instance->callback_list));
  }

  if(instance->model_callback) {
    rdr_err = rdr_detach_model_callback
      (sys, instance->model, instance->model_callback);
    assert(rdr_err == RDR_NO_ERROR);
  }

  if(instance->uniform_buffer)
    free(instance->uniform_buffer);

  if(instance->attrib_buffer)
    free(instance->attrib_buffer);

  RDR_RELEASE_OBJECT(sys, instance->model);
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
   struct rdr_model_instance** out_instance_obj)
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
  struct rdr_model_callback_desc model_cbk_desc;
  struct rdr_model_instance* instance_obj = NULL;
  struct model_instance* instance = NULL;
  enum sl_error sl_err = SL_NO_ERROR;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  bool is_model_retained = false;

  if(!sys || !model || !out_instance_obj) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  rdr_err = RDR_CREATE_OBJECT
    (sys, sizeof(struct model_instance), free_model_instance, instance_obj);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  instance = RDR_GET_OBJECT_DATA(sys, instance_obj);

  rdr_err = RDR_RETAIN_OBJECT(sys, model);
  if(rdr_err != RDR_NO_ERROR)
    goto error;
  instance->model = model;
  is_model_retained = true;

  sl_err = sl_create_linked_list
    (sizeof(struct rdr_model_instance_callback_desc),
     ALIGNOF(struct rdr_model_instance_callback_desc),
     NULL,
     &instance->callback_list);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

  model_cbk_desc.data = instance_obj;
  model_cbk_desc.func = model_callback_func;
  rdr_err = rdr_attach_model_callback
    (sys, model, &model_cbk_desc, &instance->model_callback);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = rdr_get_model_desc(sys, model, &model_desc);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = setup_model_instance_buffers(sys, instance, &model_desc);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = rdr_model_instance_transform(sys, instance_obj, identity);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = rdr_model_instance_material_density(sys, instance_obj, RDR_OPAQUE);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = rdr_model_instance_rasterizer(sys, instance_obj, &default_raster);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  if(out_instance_obj)
    *out_instance_obj = instance_obj;
  return rdr_err;

error:
  if(sys && instance_obj) {
    while(RDR_RELEASE_OBJECT(sys, instance_obj));
    instance_obj = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_free_model_instance
  (struct rdr_system* sys,
   struct rdr_model_instance* instance_obj)
{
  if(!sys || !instance_obj)
    return RDR_INVALID_ARGUMENT;

  RDR_RELEASE_OBJECT(sys, instance_obj);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_get_model_instance_uniforms
  (struct rdr_system* sys,
   struct rdr_model_instance* instance_obj,
   size_t* out_nb_uniforms,
   struct rdr_model_instance_data* uniform_data_list)
{
  struct rdr_model_desc model_desc;
  struct model_instance* instance = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  memset(&model_desc, 0, sizeof(struct rdr_model_desc));

  if(!sys || !instance_obj || !out_nb_uniforms) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  instance = RDR_GET_OBJECT_DATA(sys, instance_obj);

  rdr_err = rdr_get_model_desc(sys, instance->model, &model_desc);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = setup_uniform_data_list
    (sys,
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
  (struct rdr_system* sys,
   struct rdr_model_instance* instance_obj,
   size_t* out_nb_attribs,
   struct rdr_model_instance_data* attrib_data_list)
{
  struct rdr_model_desc model_desc;
  struct model_instance* instance = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  memset(&model_desc, 0, sizeof(struct rdr_model_desc));

  if(!sys || !instance_obj || !out_nb_attribs) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  instance = RDR_GET_OBJECT_DATA(sys, instance_obj);

  rdr_err = rdr_get_model_desc(sys, instance->model, &model_desc);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = setup_attrib_data_list
    (sys,
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
  (struct rdr_system* sys,
   struct rdr_model_instance* instance_obj,
   const float mat[16])
{
  struct model_instance* instance = NULL;

  if(!sys || !instance_obj || !mat)
    return RDR_INVALID_ARGUMENT;

  instance = RDR_GET_OBJECT_DATA(sys, instance_obj);

  instance->transform.c0 = vf4_set(mat[0], mat[1], mat[2], mat[3]);
  instance->transform.c1 = vf4_set(mat[4], mat[5], mat[6], mat[7]);
  instance->transform.c2 = vf4_set(mat[8], mat[9], mat[10], mat[11]);
  instance->transform.c3 = vf4_set(mat[12], mat[13], mat[14], mat[15]);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_get_model_instance_transform
  (struct rdr_system* sys,
   const struct rdr_model_instance* instance_obj,
   float transform[16])
{
  struct model_instance* instance = NULL;
  ALIGN(16) float mat[16];

  if(!sys || !instance_obj || !transform)
    return RDR_INVALID_ARGUMENT;

  instance = RDR_GET_OBJECT_DATA(sys, instance_obj);
  aosf44_store(mat, &instance->transform);
  transform = memcpy(transform, mat, sizeof(float) * 16);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_model_instance_material_density
  (struct rdr_system* sys,
   struct rdr_model_instance* instance_obj,
   enum rdr_material_density density)
{
  struct model_instance* instance = NULL;

  if(!sys || !instance_obj)
    return RDR_INVALID_ARGUMENT;

  instance = RDR_GET_OBJECT_DATA(sys, instance_obj);

  instance->material_density = density;
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_get_model_instance_material_density
  (struct rdr_system* sys,
   const struct rdr_model_instance* instance_obj,
   enum rdr_material_density* out_density)
{
  struct model_instance* instance = NULL;

  if(!sys || !instance_obj || !out_density)
    return RDR_INVALID_ARGUMENT;

  instance = RDR_GET_OBJECT_DATA(sys, instance_obj);

  *out_density = instance->material_density;
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_model_instance_rasterizer
  (struct rdr_system* sys,
   struct rdr_model_instance* instance_obj,
   const struct rdr_rasterizer_desc* rasterizer_desc)
{
  struct model_instance* instance = NULL;

  if(!sys || !instance_obj || !rasterizer_desc)
    return RDR_INVALID_ARGUMENT;

  instance = RDR_GET_OBJECT_DATA(sys, instance_obj);
  memcpy
    (&instance->rasterizer_desc,
     rasterizer_desc,
     sizeof(struct rdr_rasterizer_desc));

  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_get_model_instance_rasterizer
  (struct rdr_system* sys,
   const struct rdr_model_instance* instance_obj,
   struct rdr_rasterizer_desc* out_rasterizer_desc)
{
  struct model_instance* instance = NULL;

  if(!sys || !instance_obj || !out_rasterizer_desc)
    return RDR_INVALID_ARGUMENT;

  instance = RDR_GET_OBJECT_DATA(sys, instance_obj);
  memcpy
    (out_rasterizer_desc,
     &instance->rasterizer_desc,
     sizeof(struct rdr_rasterizer_desc));

  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_attach_model_instance_callback
  (struct rdr_system* sys,
   struct rdr_model_instance* instance_obj,
   const struct rdr_model_instance_callback_desc* cbk_desc,
   struct rdr_model_instance_callback** out_cbk)
{
  struct sl_node* node = NULL;
  struct model_instance* instance = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !instance_obj || !cbk_desc|| !out_cbk) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  instance = RDR_GET_OBJECT_DATA(sys, instance_obj);

  sl_err = sl_linked_list_add(instance->callback_list, cbk_desc, &node);

  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

exit:
  /* the rdr_model_instance_callback is not defined. It simply wraps the sl_node
   * data structure. */
  if(out_cbk)
    *out_cbk = (struct rdr_model_instance_callback*)node;
  return rdr_err;

error:
  if(node) {
    assert(instance != NULL);
    SL(linked_list_remove(instance->callback_list, node));
    node = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_detach_model_instance_callback
  (struct rdr_system* sys,
   struct rdr_model_instance* instance_obj,
   struct rdr_model_instance_callback* cbk)
{
  struct sl_node* node = NULL;
  struct model_instance* instance = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !instance_obj || !cbk) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  instance = RDR_GET_OBJECT_DATA(sys, instance_obj);
  node = (struct sl_node*)cbk;

  sl_err = sl_linked_list_remove(instance->callback_list, node);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

exit:
  return rdr_err;

error:
  goto exit;
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
  enum rdr_error tmp_err = RDR_NO_ERROR;
  int err = 0;

  if(!sys || !view_matrix || !proj_matrix || (nb_instances && !instance_list)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  for(i = 0; i < nb_instances; ++i) {
    struct model_instance* instance = RDR_GET_OBJECT_DATA(sys,instance_list[i]);

    if(instance == NULL) {
      rdr_err = RDR_INVALID_ARGUMENT;
      goto error;
    }

    if(instance->model != bound_mdl) {
      rdr_err = rdr_bind_model(sys, instance->model, &nb_bound_indices);
      if(rdr_err != RDR_NO_ERROR)
        goto error;
      bound_mdl = instance->model;

      rdr_err = rdr_get_model_desc(sys, bound_mdl, &bound_mdl_desc);
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
  if(sys) {
    tmp_err = rdr_bind_model(sys, NULL, NULL);
    if(tmp_err != RDR_NO_ERROR)
      rdr_err = tmp_err;
  }
  return rdr_err;

error:
  goto exit;
}

