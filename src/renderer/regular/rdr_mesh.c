#include "renderer/regular/rdr_attrib_c.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_mesh_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr_mesh.h"
#include "stdlib/sl_linked_list.h"
#include "sys/sys.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct mesh {
  struct {
    size_t stride;
    size_t offset;
    enum rb_type type;
  } attrib_list[RDR_NB_ATTRIB_USAGES];
  struct rb_buffer* data;
  struct rb_buffer* indices;
  struct sl_linked_list* callback_list;
  size_t data_size;
  size_t vertex_size;
  size_t nb_indices;
  int registered_attribs_mask;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static enum rdr_error
set_mesh_data
  (struct rdr_system* sys,
   struct mesh* mesh,
   size_t data_size,
   const void* data)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;
  assert(sys && mesh && (data || !data_size));

  if(mesh->data != NULL && (data_size == 0 || mesh->data_size < data_size)) {
    err = sys->rb.free_buffer(sys->ctxt, mesh->data);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }
    mesh->data = NULL;
    mesh->data_size = 0;
  }

  if(data_size > 0) {
    if(mesh->data == NULL) {
      const struct rb_buffer_desc desc = {
        .size = data_size,
        .target = RB_BIND_VERTEX_BUFFER,
        .usage = RB_BUFFER_USAGE_DEFAULT
      };
      err = sys->rb.create_buffer(sys->ctxt, &desc, data, &mesh->data);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }

      mesh->data_size = data_size;
    } else {
      err = sys->rb.buffer_data(sys->ctxt, mesh->data, 0, data_size, data);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }
    }
  }
  mesh->data_size = data_size;

exit:
  return rdr_err;

error:
  goto exit;
}

static enum rdr_error
set_mesh_indices
  (struct rdr_system* sys,
   struct mesh* mesh,
   size_t nb_indices,
   const unsigned int* indices)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;
  assert(sys && mesh && (indices || !nb_indices));

  if(mesh->indices != NULL
  && (nb_indices == 0 || mesh->nb_indices < nb_indices)) {
    err = sys->rb.free_buffer(sys->ctxt, mesh->indices);
    if(err != 0) {
      rdr_err = RDR_DRIVER_ERROR;
      goto error;
    }

    mesh->indices = NULL;
    mesh->nb_indices = 0;
  }

  if(nb_indices > 0) {
    const size_t indices_size = sizeof(unsigned int) * nb_indices;

    if(mesh->indices == NULL) {
      const struct rb_buffer_desc desc = {
        .size = indices_size,
        .target = RB_BIND_INDEX_BUFFER,
        .usage = RB_BUFFER_USAGE_DEFAULT
      };
      err = sys->rb.create_buffer(sys->ctxt, &desc, indices, &mesh->indices);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }

      mesh->nb_indices = nb_indices;
    } else {
      err = sys->rb.buffer_data
        (sys->ctxt, mesh->indices, 0, indices_size, indices);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }
    }
  }
  mesh->nb_indices = nb_indices;

exit:
  return rdr_err;

error:
  goto exit;
}

static bool
is_mesh_attrib_registered(struct mesh* mesh, enum rdr_attrib_usage usage)
{
  assert(mesh);
  return (mesh->registered_attribs_mask & (1 << usage)) != 0;
}

static void
unregister_all_mesh_attribs(struct mesh* mesh)
{
  assert(mesh);
  mesh->registered_attribs_mask = 0;
}

static enum rdr_error
register_mesh_attribs
  (struct rdr_system* sys UNUSED,
   struct mesh* mesh,
   size_t nb_attribs,
   const struct rdr_mesh_attrib* attrib_list)
{
  size_t i = 0;
  size_t offset = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int previous_mask = 0;

  assert(sys && mesh && (!nb_attribs || attrib_list));

  previous_mask = mesh->registered_attribs_mask;
  unregister_all_mesh_attribs(mesh);

  for(i = 0, offset = 0; i < nb_attribs; ++i) {
    const enum rdr_type attr_type = attrib_list[i].type;
    const enum rdr_attrib_usage attr_usage = attrib_list[i].usage;
    const enum rb_type rbattr_type = rdr_to_rb_type(attr_type);

    if(attr_type == RDR_UNKNOWN_TYPE) {
      rdr_err = RDR_INVALID_ARGUMENT;
      goto error;
    }

    if(attr_usage == RDR_ATTRIB_UNKNOWN) {
      rdr_err = RDR_INVALID_ARGUMENT;
      goto error;
    }

    assert(attr_usage < RDR_NB_ATTRIB_USAGES);
    mesh->attrib_list[attr_usage].stride = mesh->vertex_size;
    mesh->attrib_list[attr_usage].offset = offset;
    mesh->attrib_list[attr_usage].type = rbattr_type;

    mesh->registered_attribs_mask |= (1 << attr_usage);
    offset += sizeof_rdr_type(attr_type);
  }

exit:
  return rdr_err;

error:
  unregister_all_mesh_attribs(mesh);
  goto exit;
}

static void
free_mesh(struct rdr_system* sys, void* data)
{
  struct mesh* mesh = data;
  int err = 0;

  assert(sys && mesh);

  if(mesh->callback_list) {
    struct sl_node* node = NULL;

    SL(linked_list_head(mesh->callback_list, &node));
    assert(node == NULL);
    SL(free_linked_list(mesh->callback_list));
  }

  if(mesh->data) {
    err = sys->rb.free_buffer(sys->ctxt, mesh->data);
    assert(err == 0);
  }

  if(mesh->indices) {
    err = sys->rb.free_buffer(sys->ctxt, mesh->indices);
    assert(err == 0);
  }
}

static enum rdr_error
invoke_callbacks(struct rdr_system* sys, struct rdr_mesh* mesh_obj)
{
  struct sl_node* node = NULL;
  struct mesh* mesh = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(sys && mesh_obj);

  mesh = RDR_GET_OBJECT_DATA(sys, mesh_obj);

  for(SL(linked_list_head(mesh->callback_list, &node));
      node != NULL;
      SL(next_node(node, &node))) {
    enum rdr_error last_err = RDR_NO_ERROR;
    struct rdr_mesh_callback_desc* desc = NULL;

    SL(node_data(node, (void**)&desc));

    last_err = desc->func(sys, mesh_obj, desc->data);
    if(last_err != RDR_NO_ERROR)
      rdr_err = last_err;
  }
  return rdr_err;
}

/*******************************************************************************
 *
 * Render mesh functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rdr_error
rdr_create_mesh(struct rdr_system* sys, struct rdr_mesh** out_mesh_obj)
{
  struct rdr_mesh* mesh_obj = NULL;
  struct mesh* mesh = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !out_mesh_obj) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  rdr_err = RDR_CREATE_OBJECT
    (sys, sizeof(struct mesh), free_mesh, mesh_obj);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  mesh = RDR_GET_OBJECT_DATA(sys, mesh_obj);
  unregister_all_mesh_attribs(mesh);

  sl_err = sl_create_linked_list
    (sizeof(struct rdr_mesh_callback_desc),
     ALIGNOF(struct rdr_mesh_callback_desc),
     &mesh->callback_list);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

exit:
  if(out_mesh_obj)
    *out_mesh_obj = mesh_obj;
  return rdr_err;

error:
  if(mesh_obj)
    while(RDR_RELEASE_OBJECT(sys, mesh_obj));
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_free_mesh(struct rdr_system* sys, struct rdr_mesh* mesh_obj)
{
  if(!sys || !mesh_obj)
    return RDR_INVALID_ARGUMENT;

  RDR_RELEASE_OBJECT(sys, mesh_obj);

  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_mesh_data
  (struct rdr_system* sys,
   struct rdr_mesh* mesh_obj,
   size_t nb_attribs,
   const struct rdr_mesh_attrib* list_of_attribs,
   size_t data_size,
   const void* data)
{
  struct mesh* mesh = NULL;
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int offset = 0;

  if(!sys
  || !mesh_obj
  || (!list_of_attribs && nb_attribs)
  || (!data && data_size)
  || (!nb_attribs && data_size)
  || (!data_size && nb_attribs)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  mesh = RDR_GET_OBJECT_DATA(sys, mesh_obj);

  mesh->vertex_size = 0;
  for(i = 0, offset = 0; i < nb_attribs; ++i) {
    mesh->vertex_size += sizeof_rdr_type(list_of_attribs[i].type);
  }

  if(mesh->vertex_size > 0 && (data_size % mesh->vertex_size) != 0) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  rdr_err = set_mesh_data(sys, mesh, data_size, data);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = register_mesh_attribs(sys, mesh, nb_attribs, list_of_attribs);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  if(mesh) {
    const enum rdr_error last_err = invoke_callbacks(sys, mesh_obj);
    if(last_err != RDR_NO_ERROR)
      rdr_err = last_err;
  }

  return rdr_err;

error:
  if(sys && mesh) {
    if(mesh->data) {
      UNUSED int err = sys->rb.free_buffer(sys->ctxt, mesh->data);
      assert(err == 0);
      mesh->data = NULL;
    }
    mesh->data_size = 0;
    unregister_all_mesh_attribs(mesh);
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_mesh_indices
  (struct rdr_system* sys,
   struct rdr_mesh* mesh_obj,
   size_t nb_indices,
   const unsigned int* indices)
{
  struct mesh* mesh = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  if(!sys || !mesh_obj || (nb_indices && !indices)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  /* The meshes must be triangular. */
  if((nb_indices % 3) != 0) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  mesh = RDR_GET_OBJECT_DATA(sys, mesh_obj);

  rdr_err = set_mesh_indices(sys, mesh, nb_indices, indices);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  return rdr_err;

error:
  if(sys && mesh) {
    if(mesh->indices) {
      err = sys->rb.free_buffer(sys->ctxt, mesh->indices);
      assert(err == 0);
      mesh->indices = NULL;
    }
    mesh->nb_indices = 0;
  }
  goto exit;
}

/*******************************************************************************
 *
 * Private renderer function.
 *
 ******************************************************************************/
enum rdr_error
rdr_get_mesh_attribs
  (struct rdr_system* sys,
   struct rdr_mesh* mesh_obj,
   size_t* out_nb_attribs,
   struct rdr_mesh_attrib_desc* attrib_list)
{
  struct mesh* mesh = NULL;
  size_t i = 0;
  size_t nb_attribs = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!sys || !mesh_obj || !out_nb_attribs) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  mesh = RDR_GET_OBJECT_DATA(sys, mesh_obj);

  if(attrib_list == NULL) {
    for(i = 0; i < RDR_NB_ATTRIB_USAGES; ++i) {
      if(is_mesh_attrib_registered(mesh, i))
        ++nb_attribs;
    }
  } else {
    struct rdr_mesh_attrib_desc* dst_attr = attrib_list;
    for(i = 0; i < RDR_NB_ATTRIB_USAGES; ++i) {
      if(is_mesh_attrib_registered(mesh, i)) {
        dst_attr->stride = mesh->attrib_list[i].stride;
        dst_attr->offset = mesh->attrib_list[i].offset;
        dst_attr->type = mesh->attrib_list[i].type;
        dst_attr->usage = (enum rdr_attrib_usage)i;
        ++dst_attr;
        ++nb_attribs;
      }
    }
  }

exit:
  if(out_nb_attribs)
    *out_nb_attribs = nb_attribs;
  return rdr_err;

error:
  goto exit;
}

enum rdr_error
rdr_get_mesh_indexed_data
  (struct rdr_system* sys,
   struct rdr_mesh* mesh_obj,
   size_t* nb_indices,
   struct rb_buffer** indices,
   struct rb_buffer** data)
{
  struct mesh* mesh = NULL;

  if(!sys || !mesh_obj)
    return RDR_INVALID_ARGUMENT;

  mesh = RDR_GET_OBJECT_DATA(sys, mesh_obj);

  if(nb_indices)
    *nb_indices = mesh->nb_indices;
  if(indices)
    *indices = mesh->indices;
  if(data)
    *data = mesh->data;

  return RDR_NO_ERROR;
}

enum rdr_error
rdr_attach_mesh_callback
  (struct rdr_system* sys,
   struct rdr_mesh* mesh_obj,
   const struct rdr_mesh_callback_desc* cbk_desc,
   struct rdr_mesh_callback** out_cbk)
{
  struct sl_node* node = NULL;
  struct mesh* mesh = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !mesh_obj || !cbk_desc|| !out_cbk) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  mesh = RDR_GET_OBJECT_DATA(sys, mesh_obj);
  sl_err = sl_linked_list_add
    (mesh->callback_list, cbk_desc, &node);

  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

exit:
  /* the rdr_mesh_callback is not defined. It simply wraps the sl_node data
   * structure. */
  if(out_cbk)
    *out_cbk = (struct rdr_mesh_callback*)node;
  return rdr_err;

error:
  if(node) {
    assert(mesh != NULL);
    SL(linked_list_remove(mesh->callback_list, node));
    node = NULL;
  }
  goto exit;
}

enum rdr_error
rdr_detach_mesh_callback
  (struct rdr_system* sys,
   struct rdr_mesh* mesh_obj,
   struct rdr_mesh_callback* cbk)
{
  struct sl_node* node = NULL;
  struct mesh* mesh = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !mesh_obj || !cbk) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  mesh = RDR_GET_OBJECT_DATA(sys, mesh_obj);
  node = (struct sl_node*)cbk;

  sl_err = sl_linked_list_remove(mesh->callback_list, node);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

exit:
  return rdr_err;

error:
  goto exit;
}

