#include "renderer/regular/rdr_attrib_c.h"
#include "renderer/regular/rdr_draw_desc.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_material_c.h"
#include "renderer/regular/rdr_mesh_c.h"
#include "renderer/regular/rdr_model_c.h"
#include "renderer/regular/rdr_uniform.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_attrib.h"
#include "renderer/rdr_material.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_model.h"
#include "renderer/rdr_system.h"
#include "stdlib/sl.h"
#include "stdlib/sl_flat_set.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static const char*
builtin_attrib_name_list[] = {
  [RDR_ATTRIB_POSITION] = "rdr_position",
  [RDR_ATTRIB_NORMAL] = "rdr_normal",
  [RDR_ATTRIB_COLOR] = "rdr_color",
  [RDR_ATTRIB_TEXCOORD] = "rdr_texcoord"
};

static const char*
builtin_uniform_name_list[] = {
  [RDR_MODELVIEW_UNIFORM] = "rdr_modelview",
  [RDR_PROJECTION_UNIFORM] = "rdr_projection",
  [RDR_MODELVIEWPROJ_UNIFORM] = "rdr_modelviewproj",
  [RDR_MODELVIEW_INVTRANS_UNIFORM] = "rdr_modelview_invtrans",
  [RDR_DRAW_ID_UNIFORM] = "rdr_draw_id"
};

struct rdr_model {
  struct rdr_system* sys;
  /* Vertex array of the model and its associated informations. */
  struct rb_vertex_array* vertex_array;
  size_t nb_indices;
  int bound_attrib_index_list[RDR_NB_ATTRIB_USAGES];
  int bound_attrib_mask;
  /* Vertex array which bound only position attribs. */
  struct rb_vertex_array* vertex_pos_array;
  /* Data of the model. */
  struct rdr_mesh* mesh;
  struct rdr_material* material;
  /* List of model callbacks. */
  struct sl_flat_set* callback_set[RDR_NB_MODEL_SIGNALS];
  /* Array of instance attribs, i.e. attribs not bound to a mesh attrib. */
  struct rb_attrib** instance_attrib_list;
  size_t nb_instance_attribs;
  /* Array of material constant (e.g.: modelview/projection matrices, etc) */
  struct rdr_uniform* uniform_list;
  size_t nb_uniforms;
  /* Cumulated size in bytes of the size of the instance attrib/uniform data. */
  size_t sizeof_instance_attrib_data;
  size_t sizeof_uniform_data;
  /* Miscellaneous. */
  bool is_setuped;
  struct ref ref;
};

/*******************************************************************************
 *
 * Callbacks.
 *
 ******************************************************************************/
struct callback {
  void (*func)(struct rdr_model*, void*);
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
static void material_callback_func(struct rdr_material* mtr, void* data);
static void mesh_callback_func(struct rdr_mesh* mtr, void* data);

static enum rdr_attrib_usage
attrib_name_to_attrib_usage(const char* name)
{
  const size_t nb_builtin_attrib_names =
    sizeof(builtin_attrib_name_list) / sizeof(const char*);
  enum rdr_attrib_usage usage = RDR_ATTRIB_UNKNOWN;
  size_t i = 0;

  assert(name);

  for(i = 0; i < nb_builtin_attrib_names && usage == RDR_ATTRIB_UNKNOWN; ++i) {
    if(strcmp(name, builtin_attrib_name_list[i]) == 0)
      usage = (enum rdr_attrib_usage)i;
  }

  return usage;
}

static bool
is_attrib_bound(int bound_attrib_mask, enum rdr_attrib_usage usage)
{
  return (bound_attrib_mask & (1 << usage)) != 0;
}

static enum rdr_error
unbind_all_attribs(struct rdr_model* model)
{
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  assert(model);

  for(i = 0; i < RDR_NB_ATTRIB_USAGES; ++i) {
    if(is_attrib_bound(model->bound_attrib_mask, i) == true) {
      err = model->sys->rb.remove_vertex_attrib
        (model->vertex_array, 1, model->bound_attrib_index_list + i);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }
    }
  }
  model->bound_attrib_mask = 0;

exit:
  return rdr_err;

error:
  goto exit;
}

static enum rdr_error
bind_mtr_attrib_to_mesh_attrib
  (struct rb_attrib_desc* mtr_attr_desc,
   size_t nb_mesh_attribs,
   struct rdr_mesh_attrib_desc* mesh_attrib_list,
   struct rb_buffer* mesh_data,
   struct rdr_model* model)
{
  enum rdr_attrib_usage mtr_attr_usage = RDR_ATTRIB_UNKNOWN;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  /* Used to manage errors. */
  bool is_vertex_array_updated = false;
  bool is_vertex_pos_array_updated = false;

  assert(mtr_attr_desc
      && (!nb_mesh_attribs || mesh_attrib_list)
      && model);

  mtr_attr_usage = attrib_name_to_attrib_usage(mtr_attr_desc->name);

  if(mtr_attr_usage != RDR_ATTRIB_UNKNOWN) {
    size_t i = 0;
    bool is_attr_bound = false;

    for(i = 0; i < nb_mesh_attribs && !is_attr_bound; ++i) {
      struct rdr_mesh_attrib_desc* mesh_attr = mesh_attrib_list + i;

      if(mesh_attr->usage == mtr_attr_usage
      && mesh_attr->type == mtr_attr_desc->type) {
        struct rb_buffer_attrib buffer_attrib;
        int err = 0;

        buffer_attrib.index = mtr_attr_desc->index;
        buffer_attrib.stride = mesh_attr->stride;
        buffer_attrib.offset = mesh_attr->offset;
        buffer_attrib.type = mesh_attr->type;

        err = model->sys->rb.vertex_attrib_array
          (model->vertex_array, mesh_data, 1, &buffer_attrib);
        if(err != 0) {
          rdr_err = RDR_DRIVER_ERROR;
          goto error;
        }
        is_vertex_array_updated = true;
        if(mtr_attr_usage == RDR_ATTRIB_POSITION) {
          err = model->sys->rb.vertex_attrib_array
            (model->vertex_array, mesh_data, 1, &buffer_attrib);
          if(err != 0) {
            rdr_err = RDR_DRIVER_ERROR;
            goto error;
          }
          is_vertex_pos_array_updated = true;
        }
        is_attr_bound = true;
        model->bound_attrib_index_list[mtr_attr_usage] = mtr_attr_desc->index;
        model->bound_attrib_mask |= (1 << mesh_attr->usage);
      }
    }
  }

exit:
  return rdr_err;
error:
  if(is_vertex_array_updated) {
    RBI(&model->sys->rb, remove_vertex_attrib
      (model->vertex_array, 1, (const int[]){mtr_attr_desc->index}));
  }
  if(is_vertex_pos_array_updated) {
    RBI(&model->sys->rb, remove_vertex_attrib
    (model->vertex_pos_array, 1, (const int[]){mtr_attr_desc->index}));
  }
  goto exit;
}

static enum rdr_error
setup_model_vertex_array
  (size_t nb_mtr_attribs,
   struct rb_attrib* mtr_attrib_list[],
   size_t nb_mesh_attribs,
   struct rdr_mesh_attrib_desc* mesh_attrib_list,
   struct rb_buffer* mesh_data,
   struct rb_buffer* mesh_indices,
   struct rdr_model* mdl)
{
  struct rb_attrib** instance_attrib_list = NULL;
  size_t nb_instance_attribs = 0;
  size_t sizeof_instance_attrib_data = 0;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  assert((!nb_mtr_attribs || mtr_attrib_list)
      && (!nb_mesh_attribs || mesh_attrib_list)
      && mdl
      && !mdl->nb_instance_attribs
      && !mdl->instance_attrib_list
      && !mdl->sizeof_instance_attrib_data);

 rdr_err = unbind_all_attribs(mdl);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  if(nb_mtr_attribs) {
    instance_attrib_list = MEM_ALLOC
      (mdl->sys->allocator, sizeof(struct rb_attrib*) * nb_mtr_attribs);
    if(!instance_attrib_list) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }
  }

  for(i = 0; i < nb_mtr_attribs; ++i) {
    struct rb_attrib_desc mtr_attr_desc;
    struct rb_attrib* mtr_attr = mtr_attrib_list[i];
    const int saved_bound_attrib_mask = mdl->bound_attrib_mask;

    err = mdl->sys->rb.get_attrib_desc(mtr_attr, &mtr_attr_desc);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }

    rdr_err = bind_mtr_attrib_to_mesh_attrib
      (&mtr_attr_desc, nb_mesh_attribs, mesh_attrib_list, mesh_data, mdl);
    if(rdr_err != RDR_NO_ERROR)
      goto error;

    /* If the bound attrib mask was not updated then the mtr attrib was not
     * bound to the mesh, i.e. it is an instance attrib. */
    if(mdl->bound_attrib_mask == saved_bound_attrib_mask) {
      instance_attrib_list[nb_instance_attribs] = mtr_attr;
      sizeof_instance_attrib_data += sizeof_rb_type(mtr_attr_desc.type);
      ++nb_instance_attribs;
    }
  }

  if(nb_instance_attribs == 0 && instance_attrib_list != NULL) {
    MEM_FREE(mdl->sys->allocator, instance_attrib_list);
    instance_attrib_list = NULL;
  }

  err = mdl->sys->rb.vertex_index_array(mdl->vertex_array, mesh_indices);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

exit:
  assert(!instance_attrib_list || nb_instance_attribs);
  mdl->nb_instance_attribs = nb_instance_attribs;
  mdl->instance_attrib_list = instance_attrib_list;
  mdl->sizeof_instance_attrib_data = sizeof_instance_attrib_data;
  return rdr_err;

error:
  if(instance_attrib_list) {
    MEM_FREE(mdl->sys->allocator, instance_attrib_list);
    instance_attrib_list = NULL;
    nb_instance_attribs = 0;
  }
  sizeof_instance_attrib_data = 0;

  goto exit;
}

static enum rdr_uniform_usage
uniform_name_to_uniform_usage(const char* name)
{
  const size_t nb_builtin_uniform_names =
    sizeof(builtin_uniform_name_list) / sizeof(const char*);
  enum rdr_uniform_usage usage = RDR_REGULAR_UNIFORM;
  size_t i = 0;

  assert(name);

  for(i=0; i < nb_builtin_uniform_names && usage == RDR_REGULAR_UNIFORM; ++i) {
    if(strcmp(name, builtin_uniform_name_list[i]) == 0)
      usage = (enum rdr_uniform_usage)i;
  }

  return usage;
}

static enum rdr_error
setup_model_uniforms
  (size_t nb_uniforms,
   struct rb_uniform* uniform_list[],
   struct rdr_model* model)
{
  struct rdr_uniform* model_uniform_list = NULL;
  size_t sizeof_uniform_data = 0;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  assert((!nb_uniforms || uniform_list)
      && model
      && !model->uniform_list
      && !model->nb_uniforms
      && !model->sizeof_uniform_data);

  if(nb_uniforms) {
    model_uniform_list = MEM_ALLOC
      (model->sys->allocator, sizeof(struct rdr_uniform) * nb_uniforms);
    if(!model_uniform_list) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }
  }

  for(i = 0; i < nb_uniforms; ++i) {
    struct rb_uniform_desc uniform_desc;
    struct rb_uniform* uniform = uniform_list[i];

    err = model->sys->rb.get_uniform_desc(uniform, &uniform_desc);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }
    model_uniform_list[i].uniform = uniform;
    model_uniform_list[i].usage =
      uniform_name_to_uniform_usage(uniform_desc.name);
    sizeof_uniform_data += sizeof_rb_type(uniform_desc.type);
  }

exit:
  model->uniform_list = model_uniform_list;
  model->nb_uniforms = nb_uniforms;
  model->sizeof_uniform_data = sizeof_uniform_data;
  return rdr_err;

error:
  if(model_uniform_list) {
    MEM_FREE(model->sys->allocator, model_uniform_list);
    model_uniform_list = NULL;
    nb_uniforms = 0;
  }
  sizeof_uniform_data = 0;
  goto exit;
}

static enum rdr_error
setup_model(struct rdr_model* model)
{
  struct rdr_material_desc mtr_desc;
  struct rdr_mesh_attrib_desc* mesh_attrib_list = NULL;
  struct rb_buffer* mesh_data = NULL;
  struct rb_buffer* mesh_indices = NULL;
  size_t nb_mesh_attribs = 0;
  size_t nb_mesh_indices = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(model);

  /* Retrieve the material descriptor (attribs, uniforms, etc.) */
  rdr_err = rdr_get_material_desc(model->material, &mtr_desc);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  /* Get the mesh attribs and data. */
  rdr_err = rdr_get_mesh_attribs(model->mesh, &nb_mesh_attribs, NULL);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  if(nb_mesh_attribs != 0) {
    mesh_attrib_list = MEM_ALLOC
      (model->sys->allocator, sizeof(struct rdr_mesh_attrib_desc) * nb_mesh_attribs);
    if(!mesh_attrib_list) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }

    rdr_err = rdr_get_mesh_attribs
      (model->mesh, &nb_mesh_attribs, mesh_attrib_list);
    if(rdr_err != RDR_NO_ERROR)
      goto error;
  }

  rdr_err = rdr_get_mesh_indexed_data
    (model->mesh, &nb_mesh_indices, &mesh_indices, &mesh_data);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  /* Setup the model attribs/uniforms. */
  rdr_err = setup_model_vertex_array
    (mtr_desc.nb_attribs,
     mtr_desc.attrib_list,
     nb_mesh_attribs,
     mesh_attrib_list,
     mesh_data,
     mesh_indices,
     model);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = setup_model_uniforms
    (mtr_desc.nb_uniforms, mtr_desc.uniform_list, model);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  model->nb_indices = nb_mesh_indices;
  model->is_setuped = true;

exit:
  if(mesh_attrib_list)
    MEM_FREE(model->sys->allocator, mesh_attrib_list);

  return rdr_err;

error:
  if(model->instance_attrib_list) {
    MEM_FREE(model->sys->allocator, model->instance_attrib_list);
    model->instance_attrib_list = NULL;
    model->nb_instance_attribs = 0;
    model->sizeof_instance_attrib_data = 0;
  }
  if(model->uniform_list) {
    MEM_FREE(model->sys->allocator, model->uniform_list);
    model->uniform_list = NULL;
    model->nb_uniforms = 0;
    model->sizeof_uniform_data = 0;
  }
  goto exit;
}

static enum rdr_error
reset_model(struct rdr_model* mdl)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(mdl);

  mdl->nb_indices = 0;
  mdl->is_setuped = false;

  if(mdl->instance_attrib_list) {
    assert(mdl->nb_instance_attribs > 0);

    MEM_FREE(mdl->sys->allocator, mdl->instance_attrib_list);
    mdl->instance_attrib_list = NULL;
    mdl->nb_instance_attribs = 0;
    mdl->sizeof_instance_attrib_data = 0;
  }

  if(mdl->uniform_list) {
    assert(mdl->nb_uniforms > 0);

    MEM_FREE(mdl->sys->allocator, mdl->uniform_list);
    mdl->uniform_list = NULL;
    mdl->nb_uniforms = 0;
    mdl->sizeof_uniform_data = 0;
  }

  rdr_err = unbind_all_attribs(mdl);
  return rdr_err;
}

static void
release_model(struct ref* ref)
{
  struct rdr_system* sys = NULL;
  struct rdr_model* mdl = NULL;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  bool b = false;
  assert(ref);

  mdl = CONTAINER_OF(ref, struct rdr_model, ref);
  rdr_err = reset_model(mdl);
  assert(rdr_err == RDR_NO_ERROR);

  for(i = 0; i < RDR_NB_MODEL_SIGNALS; ++i) {
    if(mdl->callback_set[i]) {
      #ifndef NDEBUG
      size_t len = 0;
      SL(flat_set_buffer(mdl->callback_set[i], &len, NULL, NULL, NULL));
      assert(len == 0);
      #endif
      SL(free_flat_set(mdl->callback_set[i]));
    }
  }
  RDR(is_material_callback_attached
    (mdl->material,
     RDR_MATERIAL_SIGNAL_UPDATE_PROGRAM,
     material_callback_func,
     mdl,
     &b));
  if(b) {
    RDR(detach_material_callback
      (mdl->material,
       RDR_MATERIAL_SIGNAL_UPDATE_PROGRAM,
       material_callback_func,
       mdl));
  }
  RDR(is_mesh_callback_attached
    (mdl->mesh, RDR_MESH_SIGNAL_UPDATE_DATA, mesh_callback_func, mdl, &b));
  if(b) {
    RDR(detach_mesh_callback
      (mdl->mesh, RDR_MESH_SIGNAL_UPDATE_DATA, mesh_callback_func, mdl));
  }

  if(mdl->vertex_array)
    RBI(&mdl->sys->rb, vertex_array_ref_put(mdl->vertex_array));
  if(mdl->vertex_pos_array)
    RBI(&mdl->sys->rb, vertex_array_ref_put(mdl->vertex_pos_array));
  if(mdl->mesh)
    RDR(mesh_ref_put(mdl->mesh));
  if(mdl->material)
    RDR(material_ref_put(mdl->material));

  sys = mdl->sys;
  MEM_FREE(mdl->sys->allocator, mdl);
  RDR(system_ref_put(sys));
}

static void
invoke_callbacks(struct rdr_model* mdl, enum rdr_model_signal signal)
{
  struct callback* buffer = NULL;
  size_t len = 0;
  size_t i = 0;
  assert(mdl);

  SL(flat_set_buffer
    (mdl->callback_set[signal], &len, NULL, NULL, (void**)&buffer));
  for(i = 0; i < len; ++i) {
    const struct callback* clbk = buffer + i;
    clbk->func(mdl, clbk->data);
  }
}

/* Factorize the code of the material/mesh callbacks. */
static void
generic_callback_func(struct rdr_model* mdl)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  assert(mdl);

  rdr_err = reset_model(mdl);
  assert(rdr_err == RDR_NO_ERROR);
  invoke_callbacks(mdl, RDR_MODEL_SIGNAL_UPDATE_DATA);
}

static void
material_callback_func(struct rdr_material* mtr_obj UNUSED, void* data)
{
  generic_callback_func(data);
}

static void
mesh_callback_func(struct rdr_mesh* mesh UNUSED, void* data)
{
  generic_callback_func(data);
}

/******************************************************************************
 *
 * Public render model functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rdr_error
rdr_create_model
  (struct rdr_system* sys,
   struct rdr_mesh* mesh,
   struct rdr_material* material,
   struct rdr_model** out_model)
{
  struct rdr_model* model = NULL;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  int err = 0;

  if(!sys || !mesh || !material || !out_model) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  model = MEM_CALLOC(sys->allocator, 1, sizeof(struct rdr_model));
  if(!model) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  ref_init(&model->ref);
  RDR(system_ref_get(sys));
  model->sys = sys;

  for(i = 0; i < RDR_NB_MODEL_SIGNALS; ++i) {
    sl_err = sl_create_flat_set
      (sizeof(struct callback),
       ALIGNOF(struct callback),
       cmp_callbacks,
       model->sys->allocator,
       &model->callback_set[i]);
    if(sl_err != SL_NO_ERROR) {
      rdr_err = sl_to_rdr_error(sl_err);
      goto error;
    }
  }
  err = model->sys->rb.create_vertex_array
    (model->sys->ctxt, &model->vertex_array);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }
  err = model->sys->rb.create_vertex_array
    (model->sys->ctxt, &model->vertex_pos_array);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

  #define CALL(func) \
    do { \
      if(RDR_NO_ERROR != (rdr_err = func)) \
        goto error; \
    } while(0)
  CALL(rdr_model_mesh(model, mesh));
  CALL(rdr_attach_mesh_callback
    (mesh, RDR_MESH_SIGNAL_UPDATE_DATA, &mesh_callback_func, model));
  CALL(rdr_model_material(model, material));
  CALL(rdr_attach_material_callback
    (material,
     RDR_MATERIAL_SIGNAL_UPDATE_PROGRAM,
     &material_callback_func,
     model));
  #undef CALL

exit:
  if(out_model)
    *out_model = model;
  return rdr_err;

error:
  if(model) {
    RDR(model_ref_put(model));
    model = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_model_ref_get(struct rdr_model* mdl)
{
  if(!mdl)
    return RDR_INVALID_ARGUMENT;
  ref_get(&mdl->ref);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_model_ref_put(struct rdr_model* mdl)
{
  if(!mdl)
    return RDR_INVALID_ARGUMENT;
  ref_put(&mdl->ref, release_model);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_model_mesh
  (struct rdr_model* model,
   struct rdr_mesh* mesh)
{
  /* Used when an error occurs. */
  struct rdr_mesh* released_mesh = NULL;
  struct rdr_mesh* retained_mesh = NULL;

  enum rdr_error rdr_err = RDR_NO_ERROR;
  bool mesh_has_changed = false;

  if(!model || !mesh) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  if(model->mesh != mesh)  {
    mesh_has_changed = true;

    rdr_err = reset_model(model);
    if(rdr_err != RDR_NO_ERROR)
      goto error;

    if(model->mesh) {
      RDR(mesh_ref_put(model->mesh));
      released_mesh = model->mesh;
    }

    RDR(mesh_ref_get(mesh));
    retained_mesh = mesh;
    model->mesh = mesh;
  }

exit:
  if(mesh_has_changed)
    invoke_callbacks(model, RDR_MODEL_SIGNAL_UPDATE_DATA);
  return rdr_err;

error:
  if(model && released_mesh) {
    RDR(mesh_ref_get(released_mesh));
    model->mesh = released_mesh;
  }
  if(retained_mesh)
    RDR(mesh_ref_put(retained_mesh));
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_get_model_mesh(struct rdr_model* model, struct rdr_mesh** mesh)
{
  if(UNLIKELY(!model || !mesh))
    return RDR_INVALID_ARGUMENT;
  *mesh = model->mesh;
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_model_material
  (struct rdr_model* model,
   struct rdr_material* material)
{
  /* Used when an error occurs. */
  struct rdr_material* released_material = NULL;
  struct rdr_material* retained_material = NULL;

  enum rdr_error rdr_err = RDR_NO_ERROR;
  bool material_has_changed = false;
  bool is_material_linked = false;

  if(!model || !material) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  rdr_err = rdr_is_material_linked(material, &is_material_linked);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  if(!is_material_linked) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  if(material != model->material) {
    material_has_changed = true;

    rdr_err = reset_model(model);
    if(rdr_err != RDR_NO_ERROR)
      goto error;

    if(model->material) {
      RDR(material_ref_put(model->material));
      released_material = model->material;
    }

    RDR(material_ref_get(material));
    retained_material = material;
    model->material = material;
  }

exit:
  if(material_has_changed)
    invoke_callbacks(model, RDR_MODEL_SIGNAL_UPDATE_DATA);
  return rdr_err;

error:
  if(model && released_material) {
    RDR(material_ref_get(released_material));
    model->material = released_material;
  }
  if(retained_material)
    RDR(material_ref_put(retained_material));
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_get_model_material(struct rdr_model* model, struct rdr_material** mtr)
{
  if(UNLIKELY(!model || !mtr))
    return RDR_INVALID_ARGUMENT;
  *mtr = model->material;
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_bind_model
  (struct rdr_system* sys,
   struct rdr_model* model,
   size_t* out_nb_indices,
   int flag)
{
  struct rdr_material* mtr = NULL;
  struct rb_vertex_array* vertex_array = NULL;
  size_t nb_indices = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  if(!sys || (model && model->sys != sys)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  if(model && flag != RDR_BIND_NONE) {
    nb_indices = model->nb_indices;
    if(flag == RDR_BIND_ALL) {
      vertex_array = model->vertex_array;
      mtr = model->material;
    } else if(flag == RDR_BIND_ATTRIB_POSITION) {
      vertex_array = model->vertex_pos_array;
    } else {
      rdr_err = RDR_INVALID_ARGUMENT;
      goto error;
    }
    if(model->is_setuped == false) {
      rdr_err = setup_model(model);
      if(rdr_err != RDR_NO_ERROR)
        goto error;
    }
  }

  err = sys->rb.bind_vertex_array(sys->ctxt, vertex_array);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

  rdr_err = rdr_bind_material(sys, mtr);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  if(out_nb_indices)
    *out_nb_indices = nb_indices;
  return rdr_err;

error:
  goto exit;
}

/******************************************************************************
 *
 * Private render model functions.
 *
 ******************************************************************************/
enum rdr_error
rdr_get_model_desc
  (struct rdr_model* model,
   struct rdr_model_desc* desc)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!model || !desc) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  if(!model->is_setuped) {
    enum rdr_error rdr_err = setup_model(model);
    if(rdr_err != RDR_NO_ERROR)
      goto error;
  }

  desc->attrib_list = model->instance_attrib_list;
  desc->uniform_list = model->uniform_list;
  desc->nb_attribs = model->nb_instance_attribs;
  desc->nb_uniforms = model->nb_uniforms;
  desc->sizeof_attrib_data = model->sizeof_instance_attrib_data;
  desc->sizeof_uniform_data = model->sizeof_uniform_data;

exit:
  return rdr_err;

error:
  goto exit;
}

enum rdr_error
rdr_attach_model_callback
  (struct rdr_model* model,
   enum rdr_model_signal sig,
   void (*func)(struct rdr_model*, void*),
   void* data)
{
  if(!model || !func || sig >= RDR_NB_MODEL_SIGNALS)
    return  RDR_INVALID_ARGUMENT;
  SL(flat_set_insert
    (model->callback_set[sig], (struct callback[]){{func, data}}, NULL));
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_detach_model_callback
  (struct rdr_model* model,
   enum rdr_model_signal sig,
   void (*func)(struct rdr_model*, void*),
   void* data)
{
  if(!model || !func || sig >= RDR_NB_MODEL_SIGNALS)
    return RDR_INVALID_ARGUMENT;
  SL(flat_set_erase
    (model->callback_set[sig], (struct callback[]){{func, data}}, NULL));
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_is_model_callback_attached
  (struct rdr_model* model,
   enum rdr_model_signal sig,
   void (*func)(struct rdr_model*, void* data),
   void* data,
   bool* is_attached)
{
  size_t i = 0;
  size_t len = 0;

  if(!model || !func || !is_attached || sig >= RDR_NB_MODEL_SIGNALS)
    return RDR_INVALID_ARGUMENT;
  SL(flat_set_find
    (model->callback_set[sig], (struct callback[]){{func, data}}, &i));
  SL(flat_set_buffer(model->callback_set[sig], &len, NULL, NULL, NULL));
  *is_attached = (i != len);
  return RDR_NO_ERROR;
}

