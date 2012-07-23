#include "renderer/regular/rdr_attrib_c.h"
#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_mesh_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_system.h"
#include "stdlib/sl.h"
#include "stdlib/sl_flat_set.h"
#include "sys/math.h"
#include "sys/ref_count.h"
#include "sys/sys.h"
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct rdr_mesh {
  struct mesh_attrib {
    size_t stride;
    size_t offset;
    enum rb_type type;
  } attrib_list[RDR_NB_ATTRIB_USAGES];
  float min_bound[3];
  float max_bound[3];
  struct ref ref;
  struct rdr_system* sys;
  struct rb_buffer* data;
  struct rb_buffer* indices;
  struct sl_flat_set* callback_set[RDR_NB_MESH_SIGNALS];
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
  assert(mesh && (data || !data_size));

  if(mesh->data != NULL && (data_size == 0 || mesh->data_size < data_size)) {
    RBI(&mesh->sys->rb, buffer_ref_put(mesh->data));
    mesh->data = NULL;
    mesh->data_size = 0;
  }

  if(data_size > 0) {
    int err = 0;

    if(mesh->data == NULL) {
      const struct rb_buffer_desc desc = {
        .size = data_size,
        .target = RB_BIND_VERTEX_BUFFER,
        .usage = RB_USAGE_DEFAULT
      };
      err = mesh->sys->rb.create_buffer
        (mesh->sys->ctxt, &desc, data, &mesh->data);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }

      mesh->data_size = data_size;
    } else {
      err = mesh->sys->rb.buffer_data(mesh->data, 0, data_size, data);
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
  assert(mesh && (indices || !nb_indices));

  if(mesh->indices != NULL
  && (nb_indices == 0 || mesh->nb_indices < nb_indices)) {
    RBI(&mesh->sys->rb, buffer_ref_put(mesh->indices));
    mesh->indices = NULL;
    mesh->nb_indices = 0;
  }

  if(nb_indices > 0) {
    const size_t indices_size = sizeof(unsigned int) * nb_indices;
    int err = 0;

    if(mesh->indices == NULL) {
      const struct rb_buffer_desc desc = {
        .size = indices_size,
        .target = RB_BIND_INDEX_BUFFER,
        .usage = RB_USAGE_DEFAULT
      };
      err = mesh->sys->rb.create_buffer
        (mesh->sys->ctxt, &desc, indices, &mesh->indices);
      if(err != 0) {
        rdr_err = RDR_DRIVER_ERROR;
        goto error;
      }

      mesh->nb_indices = nb_indices;
    } else {
      err = mesh->sys->rb.buffer_data(mesh->indices, 0, indices_size, indices);
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

  assert(mesh && (!nb_attribs || attrib_list));

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

static FINLINE void
set_default_mesh_bounds(struct rdr_mesh* mesh)
{
  assert(mesh);
  mesh->min_bound[0] = mesh->min_bound[1] = mesh->min_bound[2] = FLT_MAX;
  mesh->max_bound[0] = mesh->max_bound[1] = mesh->max_bound[2] = -FLT_MAX;
}

static enum rdr_error
setup_mesh_bounds(struct rdr_mesh* mesh, size_t data_size, const void* data)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;

  assert(mesh && (!data_size || data));
  set_default_mesh_bounds(mesh);

  if(data_size && is_mesh_attrib_registered(mesh, RDR_ATTRIB_POSITION)) {
    const struct mesh_attrib* pos_attr = mesh->attrib_list + RDR_ATTRIB_POSITION;
    const void* pos = (const void*)((uintptr_t)data + pos_attr->offset);
    const size_t nb_pos = data_size / mesh->vertex_size;
    size_t i = 0;
    assert(data_size % mesh->vertex_size == 0);

    switch(pos_attr->type) {
      case RB_FLOAT:
        mesh->min_bound[1] = mesh->min_bound[2] = 0.f;
        mesh->max_bound[1] = mesh->max_bound[2] = 0.f;
        for(i = 0; i < nb_pos; ++i) {
          const float fpos = *(float*)pos;
          mesh->min_bound[0] = MIN(mesh->min_bound[0], fpos);
          mesh->max_bound[0] = MAX(mesh->max_bound[0], fpos);
          pos = (const void*)((uintptr_t)pos + pos_attr->stride);
        }
        break;
      case RB_FLOAT2:
        mesh->min_bound[2] = 0.f;
        mesh->max_bound[2] = 0.f;
        for(i = 0; i < nb_pos; ++i) {
          const float* fpos = (float*)pos;
          mesh->min_bound[0] = MIN(mesh->min_bound[0], fpos[0]);
          mesh->max_bound[0] = MAX(mesh->max_bound[0], fpos[0]);
          mesh->min_bound[1] = MIN(mesh->min_bound[1], fpos[1]);
          mesh->max_bound[1] = MAX(mesh->max_bound[1], fpos[1]);
          pos = (const void*)((uintptr_t)pos + pos_attr->stride);
        }
        break;
      case RB_FLOAT3:
        for(i = 0; i < nb_pos; ++i) {
          const float* fpos = (float*)pos;
          mesh->min_bound[0] = MIN(mesh->min_bound[0], fpos[0]);
          mesh->max_bound[0] = MAX(mesh->max_bound[0], fpos[0]);
          mesh->min_bound[1] = MIN(mesh->min_bound[1], fpos[1]);
          mesh->max_bound[1] = MAX(mesh->max_bound[1], fpos[1]);
          mesh->min_bound[2] = MIN(mesh->min_bound[2], fpos[2]);
          mesh->max_bound[2] = MAX(mesh->max_bound[2], fpos[2]);
          pos = (const void*)((uintptr_t)pos + pos_attr->stride);
        }
        break;
      case RB_FLOAT4:
        for(i = 0; i < nb_pos; ++i) {
          const float* fpos = (float*)pos;
          if(fpos[3] == 0.f) {
            mesh->min_bound[0] = -FLT_MAX;
            mesh->min_bound[1] = -FLT_MAX;
            mesh->min_bound[2] = -FLT_MAX;
            mesh->max_bound[0] = FLT_MAX;
            mesh->max_bound[1] = FLT_MAX;
            mesh->max_bound[2] = FLT_MAX;
            break;
          } else {
            const float rcp_w = 1.f / fpos[3];
            float coord = 0.f;
            coord = fpos[0] * rcp_w;
            mesh->min_bound[0] = MIN(mesh->min_bound[0], coord);
            mesh->max_bound[0] = MAX(mesh->max_bound[0], coord);
            coord = fpos[1] * rcp_w;
            mesh->min_bound[1] = MIN(mesh->min_bound[1], coord);
            mesh->max_bound[1] = MAX(mesh->max_bound[1], coord);
            coord = fpos[2] * rcp_w;
            mesh->min_bound[2] = MIN(mesh->min_bound[2], coord);
            mesh->max_bound[2] = MAX(mesh->max_bound[2], coord);
            pos = (const void*)((uintptr_t)pos + pos_attr->stride);
          }
        }
        break;
      default:
        rdr_err = RDR_INVALID_ARGUMENT;
        goto error;
        break;
    }
  }
exit:
  return rdr_err;
error:
  set_default_mesh_bounds(mesh);
  goto exit;
}

static void
invoke_callbacks(struct rdr_mesh* mesh, enum rdr_mesh_signal sig)
{
  struct callback* buffer = NULL;
  size_t len = 0;
  size_t i = 0;
  assert(mesh);

  SL(flat_set_buffer
    (mesh->callback_set[sig], &len, NULL, NULL, (void**)&buffer));
  for(i = 0; i < len; ++i) {
    const struct callback* clbk = buffer + i;
    clbk->func(mesh, clbk->data);
  }
}

static void
release_mesh(struct ref* ref)
{
  struct rdr_mesh* mesh = NULL;
  struct rdr_system* sys = NULL;
  size_t i = 0;
  assert(ref);

  mesh = CONTAINER_OF(ref, struct rdr_mesh, ref);

  for(i = 0; i < RDR_NB_MESH_SIGNALS; ++i) {
    if(mesh->callback_set[i]) {
      #ifndef NDEBUG
      size_t len = 0;
      SL(flat_set_buffer(mesh->callback_set[i], &len, NULL, NULL, NULL));
      assert(len == 0);
      #endif
      SL(free_flat_set(mesh->callback_set[i]));
    }
  }
  if(mesh->data)
    RBI(&mesh->sys->rb, buffer_ref_put(mesh->data));
  if(mesh->indices)
    RBI(&mesh->sys->rb, buffer_ref_put(mesh->indices));
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
  size_t i = 0;
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
  set_default_mesh_bounds(mesh);

  for(i = 0; i < RDR_NB_MESH_SIGNALS; ++i) {
    sl_err = sl_create_flat_set
      (sizeof(struct callback),
       ALIGNOF(struct callback),
       cmp_callbacks,
       mesh->sys->allocator,
       &mesh->callback_set[i]);
    if(sl_err != SL_NO_ERROR) {
      rdr_err = sl_to_rdr_error(sl_err);
      goto error;
    }
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
  invoke_callbacks(mesh, RDR_MESH_SIGNAL_UPDATE_DATA);

  rdr_err = setup_mesh_bounds(mesh, data_size, data);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  return rdr_err;

error:
  if(mesh) {
    if(mesh->data) {
      RBI(&mesh->sys->rb, buffer_ref_put(mesh->data));
      mesh->data = NULL;
    }
    mesh->data_size = 0;
    unregister_all_mesh_attribs(mesh);
    invoke_callbacks(mesh, RDR_MESH_SIGNAL_UPDATE_DATA);
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
  invoke_callbacks(mesh, RDR_MESH_SIGNAL_UPDATE_INDICES);

exit:
  if(mesh) {
  }
  return rdr_err;

error:
  if(mesh) {
    if(mesh->indices) {
      RBI(&mesh->sys->rb, buffer_ref_put(mesh->indices));
      mesh->indices = NULL;
    }
    mesh->nb_indices = 0;
    invoke_callbacks(mesh, RDR_MESH_SIGNAL_UPDATE_INDICES);
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_get_mesh_aabb
  (const struct rdr_mesh* mesh,
   float min_bound[3],
   float max_bound[3])
{
  if(UNLIKELY(!mesh || !min_bound || !max_bound))
    return RDR_INVALID_ARGUMENT;
  min_bound[0] = mesh->min_bound[0];
  min_bound[1] = mesh->min_bound[1];
  min_bound[2] = mesh->min_bound[2];
  max_bound[0] = mesh->max_bound[0];
  max_bound[1] = mesh->max_bound[1];
  max_bound[2] = mesh->max_bound[2];
  return RDR_NO_ERROR;
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
   enum rdr_mesh_signal sig,
   void (*func)(struct rdr_mesh*, void*),
   void* data)
{
  if(!mesh || !func || sig >= RDR_NB_MESH_SIGNALS)
    return  RDR_INVALID_ARGUMENT;
  SL(flat_set_insert
    (mesh->callback_set[sig], (struct callback[]){{func, data}}, NULL));
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_detach_mesh_callback
  (struct rdr_mesh* mesh,
   enum rdr_mesh_signal sig,
   void (*func)(struct rdr_mesh*, void*),
   void* data)
{
  if(!mesh || !func || sig >= RDR_NB_MESH_SIGNALS)
    return RDR_INVALID_ARGUMENT;
  SL(flat_set_erase
    (mesh->callback_set[sig], (struct callback[]){{func, data}}, NULL));
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_is_mesh_callback_attached
  (struct rdr_mesh* mesh,
   enum rdr_mesh_signal sig,
   void (*func)(struct rdr_mesh*, void* data),
   void* data,
   bool* is_attached)
{
  size_t i = 0;
  size_t len = 0;

  if(!mesh || !func || !is_attached || sig >= RDR_NB_MESH_SIGNALS)
    return RDR_INVALID_ARGUMENT;
  SL(flat_set_find
    (mesh->callback_set[sig], (struct callback[]){{func, data}}, &i));
  SL(flat_set_buffer(mesh->callback_set[sig], &len, NULL, NULL, NULL));
  *is_attached = (i != len);
  return RDR_NO_ERROR;
}

