#include "renderer/regular/rdr_attrib_c.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_material_c.h"
#include "renderer/regular/rdr_mesh_c.h"
#include "renderer/regular/rdr_model_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr_attrib.h"
#include "renderer/rdr_model.h"
#include "stdlib/sl_linked_list.h"
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
  [RDR_VIEWPROJ_UNIFORM] = "rdr_viewproj"
};

struct model {
  /* Vertex array of the model and its associated informations. */
  struct rb_vertex_array* vertex_array;
  size_t nb_indices;
  int bound_attrib_index_list[RDR_NB_ATTRIB_USAGES];
  int bound_attrib_mask;
  /* Data of the model. */
  struct rdr_mesh* mesh;
  struct rdr_material* material;
  /* Invoked when the material/mesh changed. */
  struct rdr_material_callback* material_callback;
  struct rdr_mesh_callback* mesh_callback;
  /* List of callbacks to invoke when the model desc has changed. */
  struct sl_linked_list* callback_list;
  /* Array of instance attribs, i.e. attribs not bound to a mesh attrib. */
  struct rb_attrib** instance_attrib_list;
  size_t nb_instance_attribs;
  /* Array of material constant (e.g.: modelview/projection matrices, etc) */
  struct rdr_model_uniform* uniform_list;
  size_t nb_uniforms;
  /* Cumulated size in bytes of the size of the instance attrib/uniform data. */
  size_t sizeof_instance_attrib_data;
  size_t sizeof_uniform_data;
  /* */
  bool is_setuped;
};

/*******************************************************************************
 *
 *  Helper functions.
 *
 ******************************************************************************/
static enum rdr_error
material_callback_func
  (struct rdr_system* sys,
   struct rdr_material* mtr,
   void* data);

static enum rdr_error
mesh_callback_func
  (struct rdr_system* sys,
   struct rdr_mesh* mtr,
   void* data);

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
unbind_all_attribs(struct rdr_system* sys, struct model* model)
{
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  assert(sys && model);

  for(i = 0; i < RDR_NB_ATTRIB_USAGES; ++i) {
    if(is_attrib_bound(model->bound_attrib_mask, i) == true) {
      err = sys->rb.remove_vertex_attrib
        (sys->ctxt, model->vertex_array, 1, model->bound_attrib_index_list + i);
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
  (struct rdr_system* sys,
   struct rb_attrib_desc* mtr_attr_desc,
   size_t nb_mesh_attribs,
   struct rdr_mesh_attrib_desc* mesh_attrib_list,
   struct rb_buffer* mesh_data,
   struct model* model)
{
  enum rdr_attrib_usage mtr_attr_usage = RDR_ATTRIB_UNKNOWN;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;
  bool is_attr_bound = false;

  assert(sys
      && mtr_attr_desc
      && (!nb_mesh_attribs || mesh_attrib_list)
      && model);

  mtr_attr_usage = attrib_name_to_attrib_usage(mtr_attr_desc->name);

  if(mtr_attr_usage != RDR_ATTRIB_UNKNOWN) {
    size_t i = 0;

    for(i = 0; i < nb_mesh_attribs && !is_attr_bound; ++i) {
        struct rdr_mesh_attrib_desc* mesh_attr =
          mesh_attrib_list + i;

      if(mesh_attr->usage == mtr_attr_usage
      && mesh_attr->type == mtr_attr_desc->type) {
        struct rb_buffer_attrib buffer_attrib;

        buffer_attrib.index = mtr_attr_desc->index;
        buffer_attrib.stride = mesh_attr->stride;
        buffer_attrib.offset = mesh_attr->offset;
        buffer_attrib.type = mesh_attr->type;

        err = sys->rb.vertex_attrib_array
          (sys->ctxt, model->vertex_array, mesh_data, 1, &buffer_attrib);
        if(err != 0) {
          rdr_err = RDR_DRIVER_ERROR;
          goto error;
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
  goto exit;
}

static enum rdr_error
setup_model_vertex_array
  (struct rdr_system* sys,
   size_t nb_mtr_attribs,
   struct rb_attrib* mtr_attrib_list[],
   size_t nb_mesh_attribs,
   struct rdr_mesh_attrib_desc* mesh_attrib_list,
   struct rb_buffer* mesh_data,
   struct rb_buffer* mesh_indices,
   struct model* mdl)
{
  struct rb_attrib** instance_attrib_list = NULL;
  size_t nb_instance_attribs = 0;
  size_t sizeof_instance_attrib_data = 0;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  assert(sys
      && (!nb_mtr_attribs || mtr_attrib_list)
      && (!nb_mesh_attribs || mesh_attrib_list)
      && mdl
      && !mdl->nb_instance_attribs
      && !mdl->instance_attrib_list
      && !mdl->sizeof_instance_attrib_data);

 rdr_err = unbind_all_attribs(sys, mdl);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  if(nb_mtr_attribs) {
    instance_attrib_list = malloc(sizeof(struct rb_attrib*) * nb_mtr_attribs);
    if(!instance_attrib_list) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }
  }

  for(i = 0; i < nb_mtr_attribs; ++i) {
    struct rb_attrib_desc mtr_attr_desc;
    struct rb_attrib* mtr_attr = mtr_attrib_list[i];
    const int saved_bound_attrib_mask = mdl->bound_attrib_mask;

    err = sys->rb.get_attrib_desc(sys->ctxt, mtr_attr, &mtr_attr_desc);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }

    rdr_err = bind_mtr_attrib_to_mesh_attrib
      (sys, &mtr_attr_desc, nb_mesh_attribs, mesh_attrib_list, mesh_data, mdl);
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
    free(instance_attrib_list);
    instance_attrib_list = NULL;
  }

  err = sys->rb.vertex_index_array(sys->ctxt, mdl->vertex_array, mesh_indices);
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
    free(instance_attrib_list);
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
  (struct rdr_system* sys,
   size_t nb_uniforms,
   struct rb_uniform* uniform_list[],
   struct model* model)
{
  struct rdr_model_uniform* model_uniform_list = NULL;
  size_t sizeof_uniform_data = 0;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  assert(sys
      && (!nb_uniforms || uniform_list)
      && model
      && !model->uniform_list
      && !model->nb_uniforms
      && !model->sizeof_uniform_data);

  if(nb_uniforms) {
    model_uniform_list = malloc(sizeof(struct rdr_model_uniform) * nb_uniforms);
    if(!model_uniform_list) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }
  }

  for(i = 0; i < nb_uniforms; ++i) {
    struct rb_uniform_desc uniform_desc;
    struct rb_uniform* uniform = uniform_list[i];

    err = sys->rb.get_uniform_desc(sys->ctxt, uniform, &uniform_desc);
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
    free(model_uniform_list);
    model_uniform_list = NULL;
    nb_uniforms = 0;
  }
  sizeof_uniform_data = 0;
  goto exit;
}

static enum rdr_error
setup_model(struct rdr_system* sys, struct rdr_model* model_obj)
{
  struct rdr_material_desc mtr_desc;
  struct rdr_mesh_attrib_desc* mesh_attrib_list = NULL;
  struct rb_buffer* mesh_data = NULL;
  struct rb_buffer* mesh_indices = NULL;
  struct model* model = NULL;
  size_t nb_mesh_attribs = 0;
  size_t nb_mesh_indices = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(sys && model_obj);

  model = RDR_GET_OBJECT_DATA(sys, model_obj);

  /* Retrieve the material descriptor (attribs, uniforms, etc.) */
  rdr_err = rdr_get_material_desc(sys, model->material, &mtr_desc);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  /* Get the mesh attribs and data. */
  rdr_err = rdr_get_mesh_attribs(sys, model->mesh, &nb_mesh_attribs, NULL);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  if(nb_mesh_attribs != 0) {
    mesh_attrib_list = malloc
      (sizeof(struct rdr_mesh_attrib_desc) * nb_mesh_attribs);
    if(!mesh_attrib_list) {
      rdr_err = RDR_MEMORY_ERROR;
      goto error;
    }

    rdr_err = rdr_get_mesh_attribs
      (sys, model->mesh, &nb_mesh_attribs, mesh_attrib_list);
    if(rdr_err != RDR_NO_ERROR)
      goto error;
  }

  rdr_err = rdr_get_mesh_indexed_data
    (sys, model->mesh, &nb_mesh_indices, &mesh_indices, &mesh_data);

  /* Setup the model attribs/uniforms. */
  rdr_err = setup_model_vertex_array
    (sys,
     mtr_desc.nb_attribs,
     mtr_desc.attrib_list,
     nb_mesh_attribs,
     mesh_attrib_list,
     mesh_data,
     mesh_indices,
     model);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = setup_model_uniforms
    (sys, mtr_desc.nb_uniforms, mtr_desc.uniform_list, model);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  model->nb_indices = nb_mesh_indices;
  model->is_setuped = true;

exit:
  if(mesh_attrib_list)
    free(mesh_attrib_list);

  return rdr_err;

error:
  if(model->instance_attrib_list) {
    free(model->instance_attrib_list);
    model->instance_attrib_list = NULL;
    model->nb_instance_attribs = 0;
    model->sizeof_instance_attrib_data = 0;
  }
  if(model->uniform_list) {
    free(model->uniform_list);
    model->uniform_list = NULL;
    model->nb_uniforms = 0;
    model->sizeof_uniform_data = 0;
  }
  goto exit;
}

static enum rdr_error
reset_model(struct rdr_system* sys, struct model* mdl)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(sys && mdl);

  mdl->nb_indices = 0;
  mdl->is_setuped = false;

  if(mdl->instance_attrib_list) {
    assert(mdl->nb_instance_attribs > 0);

    free(mdl->instance_attrib_list);
    mdl->instance_attrib_list = NULL;
    mdl->nb_instance_attribs = 0;
    mdl->sizeof_instance_attrib_data = 0;
  }

  if(mdl->uniform_list) {
    assert(mdl->nb_uniforms > 0);

    free(mdl->uniform_list);
    mdl->uniform_list = NULL;
    mdl->nb_uniforms = 0;
    mdl->sizeof_uniform_data = 0;
  }

  rdr_err = unbind_all_attribs(sys, mdl);
  return rdr_err;
}

static void
free_model(struct rdr_system* sys, void* data)
{
  struct model* mdl = data;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  assert(mdl);

  rdr_err = reset_model(sys, mdl);
  assert(rdr_err == RDR_NO_ERROR);

  if(mdl->callback_list) {
    struct sl_node* node = NULL;

    SL(linked_list_head(sys->sl_ctxt, mdl->callback_list, &node));
    assert(node == NULL);
    SL(free_linked_list(sys->sl_ctxt, mdl->callback_list));
  }

  if(mdl->material_callback) {
    rdr_err = rdr_detach_material_callback
      (sys, mdl->material, mdl->material_callback);
    assert(rdr_err == RDR_NO_ERROR);
  }

  if(mdl->mesh_callback) {
    rdr_err = rdr_detach_mesh_callback(sys, mdl->mesh, mdl->mesh_callback);
    assert(rdr_err == RDR_NO_ERROR);
  }

  if(mdl->vertex_array) {
    err = sys->rb.free_vertex_array(sys->ctxt, mdl->vertex_array);
    assert(err == 0);
  }

  if(mdl->mesh)
    RDR_RELEASE_OBJECT(sys, mdl->mesh);

  if(mdl->material)
    RDR_RELEASE_OBJECT(sys, mdl->material);
}

static enum rdr_error
invoke_callbacks(struct rdr_system* sys, struct rdr_model* model_obj)
{
  struct sl_node* node = NULL;
  struct model* model = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(sys&& model_obj);

  model = RDR_GET_OBJECT_DATA(sys, model_obj);

  for(SL(linked_list_head(sys->sl_ctxt, model->callback_list, &node));
      node != NULL;
      SL(next_node(sys->sl_ctxt, node, &node))) {
    enum rdr_error last_err = RDR_NO_ERROR;
    struct rdr_model_callback_desc* desc = NULL;

    SL(node_data(sys->sl_ctxt, node, (void**)&desc));

    last_err = desc->func(sys, model_obj, desc->data);
    if(last_err != RDR_NO_ERROR)
      rdr_err = last_err;
  }
  return rdr_err;
}

/* Factorise the code of the material/mesh callbacks. */
static enum rdr_error
generic_callback_func(struct rdr_system* sys, struct rdr_model* model_obj)
{
  struct model* mdl = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum rdr_error tmp_err = RDR_NO_ERROR;

  assert(model_obj);

  mdl = RDR_GET_OBJECT_DATA(sys, model_obj);

  rdr_err = reset_model(sys, mdl);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  tmp_err = invoke_callbacks(sys, model_obj);
  if(tmp_err != RDR_NO_ERROR)
    rdr_err = tmp_err;

  return rdr_err;

error:
  goto exit;
}

static enum rdr_error
material_callback_func
  (struct rdr_system* sys,
   struct rdr_material* mtr_obj UNUSED,
   void* data)
{
  return generic_callback_func(sys, data);
  return RDR_NO_ERROR;
}

static enum rdr_error
mesh_callback_func
  (struct rdr_system* sys,
   struct rdr_mesh* mtr_obj UNUSED,
   void* data)
{
  return generic_callback_func(sys, data);
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
   struct rdr_model** out_model_obj)
{
  struct rdr_mesh_callback_desc mesh_cbk_desc;
  struct rdr_material_callback_desc mtr_cbk_desc;
  struct rdr_model* model_obj = NULL;
  struct model* model = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  int err = 0;

  if(!sys || !mesh || !material || !out_model_obj) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  rdr_err = RDR_CREATE_OBJECT(sys, sizeof(struct model), free_model, model_obj);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  model = RDR_GET_OBJECT_DATA(sys, model_obj);

  sl_err = sl_create_linked_list
    (sys->sl_ctxt,
     sizeof(struct rdr_model_callback_desc),
     ALIGNOF(struct rdr_model_callback_desc),
     &model->callback_list);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

  err = sys->rb.create_vertex_array(sys->ctxt, &model->vertex_array);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

  rdr_err = rdr_model_mesh(sys, model_obj, mesh);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  mesh_cbk_desc.data = model_obj;
  mesh_cbk_desc.func = mesh_callback_func;
  rdr_err = rdr_attach_mesh_callback
    (sys, mesh, &mesh_cbk_desc, &model->mesh_callback);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = rdr_model_material(sys, model_obj, material);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  mtr_cbk_desc.data = model_obj;
  mtr_cbk_desc.func = material_callback_func;
  rdr_err = rdr_attach_material_callback
    (sys, material, &mtr_cbk_desc, &model->material_callback);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  if(out_model_obj)
    *out_model_obj = model_obj;
  return rdr_err;

error:
  if(sys && model_obj) {
    while(RDR_RELEASE_OBJECT(sys, model_obj));
    model_obj = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_free_model(struct rdr_system* sys, struct rdr_model* mdl)
{
  if(!sys || !mdl)
    return RDR_INVALID_ARGUMENT;

  RDR_RELEASE_OBJECT(sys, mdl);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_model_mesh
  (struct rdr_system* sys,
   struct rdr_model* model_obj,
   struct rdr_mesh* mesh)
{
  /* Used when an error occurs. */
  struct rdr_mesh* released_mesh = NULL;
  struct rdr_mesh* retained_mesh = NULL;

  struct model* model = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum rdr_error tmp_err = RDR_NO_ERROR;
  bool mesh_has_changed = false;

  if(!sys || !model_obj || !mesh) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  model = RDR_GET_OBJECT_DATA(sys, model_obj);

  if(model->mesh != mesh)  {
    mesh_has_changed = true;

    rdr_err = reset_model(sys, model);
    if(rdr_err != RDR_NO_ERROR)
      goto error;

    if(model->mesh) {
      RDR_RELEASE_OBJECT(sys, model->mesh);
      released_mesh = model->mesh;
    }

    rdr_err = RDR_RETAIN_OBJECT(sys, mesh);
    if(rdr_err != RDR_NO_ERROR)
      goto error;
    retained_mesh = mesh;

    model->mesh = mesh;
  }

exit:
  if(mesh_has_changed) {
    tmp_err = invoke_callbacks(sys, model_obj);
    if(tmp_err != RDR_NO_ERROR)
      rdr_err = tmp_err;
  }
  return rdr_err;

error:
  if(model && released_mesh) {
    RDR_RETAIN_OBJECT(sys, released_mesh);
    model->mesh = released_mesh;
  }
  if(retained_mesh)
    RDR_RELEASE_OBJECT(sys, retained_mesh);
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_model_material
  (struct rdr_system* sys,
   struct rdr_model* model_obj,
   struct rdr_material* material)
{
  /* Used when an error occurs. */
  struct rdr_material* released_material = NULL;
  struct rdr_material* retained_material = NULL;

  struct model* model = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum rdr_error tmp_err = RDR_NO_ERROR;
  bool material_has_changed = false;
  bool is_material_linked = false;

  if(!sys || !model_obj || !material) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  rdr_err = rdr_is_material_linked(sys, material, &is_material_linked);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  if(!is_material_linked) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  model = RDR_GET_OBJECT_DATA(sys, model_obj);

  if(material != model->material) {
    material_has_changed = true;

    rdr_err = reset_model(sys, model);
    if(rdr_err != RDR_NO_ERROR)
      goto error;

    if(model->material) {
      RDR_RELEASE_OBJECT(sys, model->material);
      released_material = model->material;
    }

    rdr_err = RDR_RETAIN_OBJECT(sys, material);
    if(rdr_err != RDR_NO_ERROR)
      goto error;
    retained_material = material;

    model->material = material;
  }

exit:
  if(material_has_changed) {
    tmp_err = invoke_callbacks(sys, model_obj);
    if(tmp_err != RDR_NO_ERROR)
      rdr_err = tmp_err;
  }
  return rdr_err;

error:
  if(model && released_material) {
    RDR_RETAIN_OBJECT(sys, released_material);
    model->material = released_material;
  }
  if(retained_material)
    RDR_RELEASE_OBJECT(sys, retained_material);

  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_bind_model
  (struct rdr_system* sys,
   struct rdr_model* model_obj,
   size_t* out_nb_indices)
{
  struct rdr_material* mtr = NULL;
  struct rb_vertex_array* vertex_array = NULL;
  size_t nb_indices = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  if(!sys) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  if(model_obj) {
    struct model* model = RDR_GET_OBJECT_DATA(sys, model_obj);

    mtr = model->material;
    nb_indices = model->nb_indices;
    vertex_array = model->vertex_array;

    if(model->is_setuped == false) {
      rdr_err = setup_model(sys, model_obj);
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
  (struct rdr_system* sys,
   struct rdr_model* model_obj,
   struct rdr_model_desc* desc)
{
  struct model* model = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!sys || !model_obj || !desc) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  model = RDR_GET_OBJECT_DATA(sys, model_obj);

  if(!model->is_setuped) {
    enum rdr_error rdr_err = setup_model(sys, model_obj);
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

extern enum rdr_error
rdr_attach_model_callback
  (struct rdr_system* sys,
   struct rdr_model* mdl_obj,
   const struct rdr_model_callback_desc* cbk_desc,
   struct rdr_model_callback** out_cbk)
{
  struct sl_node* node = NULL;
  struct model* mdl = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !mdl_obj || !cbk_desc|| !out_cbk) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  mdl = RDR_GET_OBJECT_DATA(sys, mdl_obj);
  sl_err = sl_linked_list_add
    (sys->sl_ctxt, mdl->callback_list, cbk_desc, &node);

  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

exit:
  /* the rdr_model_callback is not defined. It simply wrap the sl_node
   * data structure. */
  if(out_cbk)
    *out_cbk = (struct rdr_model_callback*)node;
  return rdr_err;

error:
  if(node) {
    assert(mdl != NULL);
    SL(linked_list_remove(sys->sl_ctxt, mdl->callback_list, node));
    node = NULL;
  }
  goto exit;
}

enum rdr_error
rdr_detach_model_callback
  (struct rdr_system* sys,
   struct rdr_model* mdl_obj,
   struct rdr_model_callback* cbk)
{
  struct sl_node* node = NULL;
  struct model* mdl = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !mdl_obj || !cbk) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  mdl = RDR_GET_OBJECT_DATA(sys, mdl_obj);
  node = (struct sl_node*)cbk;

  sl_err = sl_linked_list_remove(sys->sl_ctxt, mdl->callback_list, node);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

exit:
  return rdr_err;

error:
  goto exit;
}

