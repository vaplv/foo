#include "renderer/regular/rdr_attrib_c.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_mesh_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_system.h"
#include "stdlib/sl.h"
#include "stdlib/sl_set.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct rdr_mesh {
  struct {
    size_t stride;
    size_t offset;
    enum rb_type type;
  } attrib_list[RDR_NB_ATTRIB_USAGES];
  struct ref ref;
  struct rdr_system* sys;
  struct rb_buffer* data;
  struct rb_buffer* indices;
  struct sl_set* callback_set;
  size_t data_size;
  size_t vertex_size;
  size_t nb_indices;
  int registered_attribs_mask;
};

/*******************************************************************************
 *
 * Callbacks.
 *
 ******************************************************************************/
struct callback {
  void (*func)(struct rdr_mesh*, void*);
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
set_mesh_data(struct rdr_mesh* mesh, size_t data_size, const void* data)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;
  assert(mesh && (data || !data_size));

  if(mesh->data != NULL && (data_size == 0 || mesh->data_size < data_size)) {
    err = mesh->sys->rb.free_buffer(mesh->sys->ctxt, mesh->data);
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
      err = mesh->sys->rb.create_buffer(mesh->sys->ctxt, &desc, data, &mesh->data);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }

      mesh->data_size = data_size;
    } else {
      err = mesh->sys->rb.buffer_data(mesh->sys->ctxt, mesh->data, 0, data_size, data);
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
  (struct rdr_mesh* mesh,
   size_t nb_indices,
   const unsigned int* indices)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;
  assert(mesh && (indices || !nb_indices));

  if(mesh->indices != NULL
  && (nb_indices == 0 || mesh->nb_indices < nb_indices)) {
    err = mesh->sys->rb.free_buffer(mesh->sys->ctxt, mesh->indices);
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
      err = mesh->sys->rb.create_buffer(mesh->sys->ctxt, &desc, indices, &mesh->indices);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }

      mesh->nb_indices = nb_indices;
    } else {
      err = mesh->sys->rb.buffer_data
        (mesh->sys->ctxt, mesh->indices, 0, indices_size, indices);
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
is_mesh_attrib_registered(struct rdr_mesh* mesh, enum rdr_attrib_usage usage)
{
  assert(mesh);
  return (mesh->registered_attribs_mask & (1 << usage)) != 0;
}

static void
unregister_all_mesh_attribs(struct rdr_mesh* mesh)
{
  assert(mesh);
  mesh->registered_attribs_mask = 0;
}

static enum rdr_error
register_mesh_attribs
  (struct rdr_mesh* mesh,
   size_t nb_attribs,
   const struct rdr_mesh_attrib* attrib_list)
{
  size_t i = 0;
  size_t offset = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int previous_mask = 0;

  assert(mesh && (!nb_attribs || attrib_list));

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

    assert((int)attr_usage < RDR_NB_ATTRIB_USAGES);
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
invoke_callbacks(struct rdr_mesh* mesh)
{
  struct callback* buffer = NULL;
  size_t len = 0;
  size_t i = 0;
  assert(mesh);

  SL(set_buffer(mesh->callback_set, &len, NULL, NULL, (void**)&buffer));
  for(i = 0; i < len; ++i) {
    struct callback* clbk = buffer + i;
    clbk->func(mesh, clbk->data);
  }
}

static void
release_mesh(struct ref* ref)
{
  struct rdr_mesh* mesh = NULL;
  struct rdr_system* sys = NULL;
  int err = 0;
  assert(ref);

  mesh = CONTAINER_OF(ref, struct rdr_mesh, ref);

  if(mesh->callback_set) {
    size_t len = 0;
    SL(set_buffer(mesh->callback_set, &len, NULL, NULL, NULL));
    assert(len == 0);
    SL(free_set(mesh->callback_set));
  }
  if(mesh->data) {
    err = mesh->sys->rb.free_buffer(mesh->sys->ctxt, mesh->data);
    assert(err == 0);
  }
  if(mesh->indices) {
    err = mesh->sys->rb.free_buffer(mesh->sys->ctxt, mesh->indices);
    assert(err == 0);
  }
  sys = mesh->sys;
  MEM_FREE(sys->allocator, mesh);
  RDR(system_ref_put(sys));
}

/*******************************************************************************
 *
 * Render mesh functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rdr_error
rdr_create_mesh(struct rdr_system* sys, struct rdr_mesh** out_mesh)
{
  struct rdr_mesh* mesh = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !out_mesh) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  mesh = MEM_CALLOC(sys->allocator, 1, sizeof(struct rdr_mesh));
  if(!mesh) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  ref_init(&mesh->ref);
  mesh->sys = sys;
  RDR(system_ref_get(sys));
  unregister_all_mesh_attribs(mesh);

  sl_err = sl_create_set
    (sizeof(struct callback),
     ALIGNOF(struct callback),
     cmp_callbacks,
     mesh->sys->allocator,
     &mesh->callback_set);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

exit:
  if(out_mesh)
    *out_mesh = mesh;
  return rdr_err;

error:
  if(mesh) {
    RDR(mesh_ref_put(mesh));
    mesh = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_mesh_ref_get(struct rdr_mesh* mesh)
{
  if(!mesh)
    return RDR_INVALID_ARGUMENT;
  ref_get(&mesh->ref);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_mesh_ref_put(struct rdr_mesh* mesh)
{
  if(!mesh)
    return RDR_INVALID_ARGUMENT;
  ref_put(&mesh->ref, release_mesh);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_mesh_data
  (struct rdr_mesh* mesh,
   size_t nb_attribs,
   const struct rdr_mesh_attrib* list_of_attribs,
   size_t data_size,
   const void* data)
{
  size_t i = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int offset = 0;

  if(!mesh
  || (!list_of_attribs && nb_attribs)
  || (!data && data_size)
  || (!nb_attribs && data_size)
  || (!data_size && nb_attribs)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  mesh->vertex_size = 0;
  for(i = 0, offset = 0; i < nb_attribs; ++i) {
    mesh->vertex_size += sizeof_rdr_type(list_of_attribs[i].type);
  }
  if(mesh->vertex_size > 0 && (data_size % mesh->vertex_size) != 0) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  rdr_err = set_mesh_data(mesh, data_size, data);
  if(rdr_err != RDR_NO_ERROR)
    goto error;
  rdr_err = register_mesh_attribs(mesh, nb_attribs, list_of_attribs);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  if(mesh)
    invoke_callbacks(mesh);
  return rdr_err;

error:
  if(mesh) {
    if(mesh->data) {
      UNUSED int err = mesh->sys->rb.free_buffer(mesh->sys->ctxt, mesh->data);
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
  (struct rdr_mesh* mesh,
   size_t nb_indices,
   const unsigned int* indices)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  if(!mesh || (nb_indices && !indices)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  /* The meshes must be triangular. */
  if((nb_indices % 3) != 0) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  rdr_err = set_mesh_indices(mesh, nb_indices, indices);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  return rdr_err;

error:
  if(mesh) {
    if(mesh->indices) {
      err = mesh->sys->rb.free_buffer(mesh->sys->ctxt, mesh->indices);
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
  (struct rdr_mesh* mesh,
   size_t* out_nb_attribs,
   struct rdr_mesh_attrib_desc* attrib_list)
{
  size_t i = 0;
  size_t nb_attribs = 0;
  enum rdr_error rdr_err = RDR_NO_ERROR;

  if(!mesh || !out_nb_attribs) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
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
  (struct rdr_mesh* mesh,
   size_t* nb_indices,
   struct rb_buffer** indices,
   struct rb_buffer** data)
{
  if(!mesh)
    return RDR_INVALID_ARGUMENT;

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
  (struct rdr_mesh* mesh,
   void (*func)(struct rdr_mesh*, void*),
   void* data)
{
  if(!mesh || !func)
    return  RDR_INVALID_ARGUMENT;
  SL(set_insert(mesh->callback_set, (struct callback[]){{func, data}}));
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_detach_mesh_callback
  (struct rdr_mesh* mesh,
   void (*func)(struct rdr_mesh*, void*),
   void* data)
{
  if(!mesh || !func)
    return RDR_INVALID_ARGUMENT;
  SL(set_remove(mesh->callback_set, (struct callback[]){{func, data}}));
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_is_mesh_callback_attached
  (struct rdr_mesh* mesh,
   void (*func)(struct rdr_mesh*, void* data),
   void* data,
   bool* is_attached)
{
  size_t i = 0;
  size_t len = 0;

  if(!mesh || !func || !is_attached)
    return RDR_INVALID_ARGUMENT;
  SL(set_find(mesh->callback_set, (struct callback[]){{func, data}}, &i));
  SL(set_buffer(mesh->callback_set, &len, NULL, NULL, NULL));
  *is_attached = (i != len);
  return RDR_NO_ERROR;
}

