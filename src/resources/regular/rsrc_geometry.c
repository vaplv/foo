#include "resources/regular/rsrc_context_c.h"
#include "resources/regular/rsrc_error_c.h"
#include "resources/regular/rsrc_wavefront_obj_c.h"
#include "resources/rsrc_geometry.h"
#include "stdlib/sl_hash_table.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct primitive_set {
  struct sl_vector* data_list; /* vector of float. */
  struct sl_vector* index_list; /* vector of unsigned int. */
  struct sl_vector* attrib_list; /* vector of struct rsrc_attrib. */
  enum rsrc_primitive_type primitive_type;
};

struct rsrc_geometry {
  struct sl_vector* primitive_set_list; /* vector of vector of primitive_set. */
  struct sl_hash_table* hash_table; /* Internal hash table. */
};

/*******************************************************************************
 *
 * Vertex pair data type.
 *
 ******************************************************************************/
struct pair {
  struct vvtvn {
    size_t v;
    size_t vt;
    size_t vn;
  } key;
  unsigned int index;
};

static const void*
get_key(const void* pair)
{
  return (const void*)(&((const struct pair*)pair)->key);
}

static int
compare(const void* p0, const void* p1)
{
  const struct vvtvn* a = (const struct vvtvn*)p0;
  const struct vvtvn* b = (const struct vvtvn*)p1;
  const int eq_v = a->v == b->v;
  const int eq_vt = a->vt == b->vt;
  const int lt =
    (a->v < b->v) | ((eq_v & (a->vt < b->vt)) | (eq_vt & (a->vn < b->vn)));
  const int gt =
    (a->v > b->v) | ((eq_v & (a->vt > b->vt)) | (eq_vt & (a->vn > b->vn)));

  return -lt | gt;
}

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static enum rsrc_error
build_triangle_list
  (struct rsrc_context* ctxt UNUSED,
   const float (*pos)[3],
   const float (*nor)[3],
   const float (*tex)[3],
   const struct rsrc_wavefront_obj_range* face_range,
   struct sl_vector* faces[],
   struct sl_hash_table* tbl,
   struct sl_vector** out_data,
   struct sl_vector** out_indices,
   struct sl_vector** out_attribs)
{
  struct sl_vector* data = NULL;
  struct sl_vector* indices = NULL;
  struct sl_vector* attribs = NULL;
  size_t face_id = 0;
  unsigned int nb_indices = 0;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  assert(ctxt
      && pos
      && nor
      && tex
      && face_range
      && faces
      && tbl
      && out_data
      && out_indices
      && out_attribs
      && face_range->begin < face_range->end);

  #define SL_FUNC(func) \
    do { \
      sl_err = sl_##func; \
      if(sl_err != SL_NO_ERROR) { \
       err = sl_to_rsrc_error(sl_err); \
       goto error; \
      } \
    } while(0)

  SL_FUNC(hash_table_clear(tbl));
  SL_FUNC(hash_table_resize
    (tbl, (face_range->end - face_range->begin) / 2));

  SL_FUNC(create_vector
    (sizeof(unsigned int), ALIGNOF(unsigned int), ctxt->allocator, &indices));
  SL_FUNC(create_vector
    (sizeof(float[8]), ALIGNOF(float[8]), ctxt->allocator, &data));
  SL_FUNC(create_vector
    (sizeof(struct rsrc_attrib),
     ALIGNOF(struct rsrc_attrib),
     ctxt->allocator,
     &attribs));

  SL_FUNC(vector_reserve(attribs, 3));
  SL_FUNC(vector_push_back
    (attribs, (struct rsrc_attrib[]) {{RSRC_FLOAT3, RSRC_ATTRIB_POSITION}}));
  SL_FUNC(vector_push_back
    (attribs, (struct rsrc_attrib[]) {{RSRC_FLOAT3, RSRC_ATTRIB_NORMAL}}));
  SL_FUNC(vector_push_back
    (attribs, (struct rsrc_attrib[]) {{RSRC_FLOAT2, RSRC_ATTRIB_TEXCOORD}}));

  for(face_id = face_range->begin;
      face_id < face_range->end;
      ++face_id) {
    struct rsrc_wavefront_obj_face* face_verts = NULL;
    size_t nb_verts = 0, vert_id = 0;

    SL_FUNC(vector_buffer
       (faces[face_id],
        &nb_verts,
        NULL,
        NULL,
        (void**)&face_verts));

    /* We expect triangular faces. */
    if(nb_verts != 3) {
      err = RSRC_PARSING_ERROR;
      goto error;
    }

    for(vert_id = 0; vert_id < nb_verts; ++vert_id) {
      struct pair* pair = NULL;
      const struct vvtvn key = {
        .v = face_verts[vert_id].v,
        .vt = face_verts[vert_id].vt,
        .vn = face_verts[vert_id].vn
      };

      SL_FUNC(hash_table_find(tbl, (const void*)&key, (void**)&pair));
      if(pair != NULL) {
        SL_FUNC(vector_push_back(indices, (void*)&pair->index));
      } else {
        float face_vertex[8];
        memset(&face_vertex, 0, sizeof(face_vertex));

        /* NOTE: the obj indexing starts at 1. */
        if(key.v > 0)
          memcpy(face_vertex + 0, pos[key.v - 1], sizeof(float[3]));
        if(key.vn > 0)
          memcpy(face_vertex + 3, nor[key.vn - 1], sizeof(float[3]));
        if(key.vt > 0) {
          if(tex[key.vt - 1][2] != 0.f) {  /* We expect 2d tex coords. */
            err = RSRC_PARSING_ERROR;
            goto error;
          }
          memcpy(face_vertex + 6, tex[key.vt - 1], sizeof(float[2]));
        }

        SL_FUNC(vector_push_back(data, (void*)&face_vertex));
        SL_FUNC(vector_push_back(indices, (void*)&nb_indices));
        SL_FUNC(hash_table_insert(tbl, (struct pair[]){{key, nb_indices}}));

        if(nb_indices == UINT_MAX) {
          err = RSRC_OVERFOW_ERROR;
          goto error;
        }
        ++nb_indices;
      }
    }
  }
  #undef SL_FUNC

exit:
  *out_data = data;
  *out_indices = indices;
  *out_attribs = attribs;
  return err;

error:
  if(data) {
    SL(free_vector(data));
    data = NULL;
  }
  if(indices) {
    SL(free_vector(indices));
    indices = NULL;
  }
  if(attribs){
    SL(free_vector(attribs));
    attribs = NULL;
  }
  goto exit;
}

/*******************************************************************************
 *
 * Implementation of the primitive list functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rsrc_error
rsrc_create_geometry
  (struct rsrc_context* ctxt,
   struct rsrc_geometry** out_geom)
{
  struct rsrc_geometry* geom = NULL;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!ctxt || !out_geom) {
    err = RSRC_INVALID_ARGUMENT;
    goto error;
  }
  geom = MEM_CALLOC_I(ctxt->allocator, 1, sizeof(struct rsrc_geometry));
  if(!geom) {
    err = RSRC_MEMORY_ERROR;
    goto error;
  }

  sl_err = sl_create_vector
    (sizeof(struct primitive_set),
     ALIGNOF(struct primitive_set),
     ctxt->allocator,
     &geom->primitive_set_list);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }

  sl_err = sl_create_hash_table
    (sizeof(struct pair),
     ALIGNOF(struct pair),
     sizeof(struct vvtvn),
     compare,
     get_key,
     ctxt->allocator,
     &geom->hash_table);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }

exit:
  if(out_geom)
    *out_geom = geom;
  return err;

error:
  if(geom) {
    if(geom->primitive_set_list)
      SL(free_vector(geom->primitive_set_list));
    if(geom->hash_table)
      SL(free_hash_table(geom->hash_table));
    MEM_FREE_I(ctxt->allocator, geom);
    geom = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_free_geometry
  (struct rsrc_context* ctxt,
   struct rsrc_geometry* geom)
{
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!ctxt || !geom) {
    err = RSRC_INVALID_ARGUMENT;
    goto error;
  }

  err = rsrc_clear_geometry(ctxt, geom);
  if(err != RSRC_NO_ERROR)
    goto error;

  if(geom->primitive_set_list) {
    sl_err = sl_free_vector(geom->primitive_set_list);
    if(sl_err != SL_NO_ERROR) {
      err = sl_to_rsrc_error(sl_err);
      goto error;
    }
  }
  if(geom->hash_table) {
    sl_err = sl_free_hash_table(geom->hash_table);
    if(sl_err != SL_NO_ERROR) {
      err = sl_to_rsrc_error(sl_err);
      goto error;
    }
  }
  MEM_FREE_I(ctxt->allocator, geom);

exit:
  return err;

error:
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_clear_geometry
  (struct rsrc_context* ctxt,
   struct rsrc_geometry* geom)
{
  struct primitive_set* prim_set = NULL;
  size_t len = 0;
  size_t i = 0;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!ctxt || !geom) {
    err = RSRC_INVALID_ARGUMENT;
    goto error;
  }

  if(geom->primitive_set_list) {
    sl_err = sl_vector_buffer
      (geom->primitive_set_list,
       &len,
       NULL,
       NULL,
       (void**)&prim_set);
    if(sl_err != SL_NO_ERROR) {
      err = sl_to_rsrc_error(sl_err);
      goto error;
    }

    for(i = 0; i < len; ++i) {
      assert(prim_set[i].data_list != NULL);
      sl_err = sl_free_vector(prim_set[i].data_list);
      if(sl_err != SL_NO_ERROR) {
        err = sl_to_rsrc_error(sl_err);
        goto error;
      }

      assert(prim_set[i].index_list != NULL);
      sl_err = sl_free_vector(prim_set[i].index_list);
      if(sl_err != SL_NO_ERROR) {
        err = sl_to_rsrc_error(sl_err);
        goto error;
      }

      assert(prim_set[i].attrib_list != NULL);
      sl_err = sl_free_vector(prim_set[i].attrib_list);
      if(sl_err != SL_NO_ERROR) {
        err = sl_to_rsrc_error(sl_err);
        goto error;
      }
    }

    sl_err = sl_clear_vector(geom->primitive_set_list);
    if(sl_err != SL_NO_ERROR) {
      err = sl_to_rsrc_error(sl_err);
      goto error;
    }
  }

exit:
  return err;

error:
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_geometry_from_wavefront_obj
  (struct rsrc_context* ctxt,
   struct rsrc_geometry* geom,
   const struct rsrc_wavefront_obj* wobj)
{
  struct primitive_set prim_set;
  const float (*wobj_pos)[3] = NULL;
  const float (*wobj_nor)[3] = NULL;
  const float (*wobj_tex)[3] = NULL;
  const struct rsrc_wavefront_obj_group* wobj_groups = NULL;
  struct sl_vector** wobj_faces = NULL;
  size_t group_id = 0;
  size_t nb_groups = 0;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  memset(&prim_set, 0, sizeof(prim_set));

  if(!ctxt || !geom || !wobj) {
    err = RSRC_INVALID_ARGUMENT;
    goto error;
  }

  #define VECTOR_BUFFER(vec, len, buf) \
    do { \
      sl_err = sl_vector_buffer(vec, (len), NULL, NULL, (void**)buf); \
      if(sl_err != SL_NO_ERROR) { \
        err = sl_to_rsrc_error(sl_err); \
        goto error; \
      } \
    } while(0)
  VECTOR_BUFFER(wobj->position_list, NULL, &wobj_pos);
  VECTOR_BUFFER(wobj->normal_list, NULL, &wobj_nor);
  VECTOR_BUFFER(wobj->texcoord_list, NULL, &wobj_tex);
  VECTOR_BUFFER(wobj->face_list, NULL, &wobj_faces);
  VECTOR_BUFFER(wobj->group_list, &nb_groups, &wobj_groups);
  #undef VECTOR_BUFFER

  err = rsrc_clear_geometry(ctxt, geom);
  if(err != RSRC_NO_ERROR)
    goto error;

  for(group_id = 0; group_id < nb_groups; ++group_id) {
    const struct rsrc_wavefront_obj_range* face_range =
      &wobj_groups[group_id].face_range;

    if(face_range->begin == face_range->end)
      continue;

    memset(&prim_set, 0, sizeof(prim_set));
    prim_set.primitive_type = RSRC_TRIANGLE;
    err = build_triangle_list
      (ctxt,
       wobj_pos,
       wobj_nor,
       wobj_tex,
       face_range,
       wobj_faces,
       geom->hash_table,
       &prim_set.data_list,
       &prim_set.index_list,
       &prim_set.attrib_list);
    if(err != RSRC_NO_ERROR)
      goto error;

    sl_err = sl_vector_push_back(geom->primitive_set_list, (void*)&prim_set);
    if(sl_err != SL_NO_ERROR) {
      err = sl_to_rsrc_error(sl_err);
      goto error;
    }
  }

exit:
  return err;

error:
  if(prim_set.data_list)
    SL(free_vector(prim_set.data_list));
  if(prim_set.index_list)
    SL(free_vector(prim_set.index_list));
  if(prim_set.attrib_list)
    SL(free_vector(prim_set.attrib_list));
  {
    UNUSED const enum rsrc_error tmp_err = rsrc_clear_geometry(ctxt, geom);
    assert(tmp_err == RSRC_NO_ERROR);
  }
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_get_primitive_set_count
  (struct rsrc_context* ctxt,
   const struct rsrc_geometry* geom,
   size_t* out_nb_prim_list)
{
  enum sl_error sl_err = SL_NO_ERROR;

  if(!ctxt || !geom || !out_nb_prim_list)
    return RSRC_INVALID_ARGUMENT;

  sl_err = sl_vector_length(geom->primitive_set_list, out_nb_prim_list);
  if(sl_err != SL_NO_ERROR)
    return sl_to_rsrc_error(sl_err);

  return RSRC_NO_ERROR;
}

EXPORT_SYM enum rsrc_error
rsrc_get_primitive_set
  (struct rsrc_context* ctxt,
   const struct rsrc_geometry* geom,
   size_t id,
   struct rsrc_primitive_set* primitive_set)
{
  struct primitive_set* prim_set_lst = NULL;
  void* buffer = NULL;
  size_t size = 0;
  size_t len = 0;
  enum rsrc_error err = RSRC_NO_ERROR;

  if(!ctxt || !geom || !primitive_set) {
    err = RSRC_INVALID_ARGUMENT;
    goto error;
  }

  SL(vector_buffer
     (geom->primitive_set_list,
      &len,
      NULL,
      NULL,
      (void**)&prim_set_lst));

  if(id >= len) {
    err = RSRC_INVALID_ARGUMENT;
    goto error;
  }

  SL(vector_buffer
     (prim_set_lst[id].data_list, &len, &size, NULL, &buffer));
  primitive_set->data = buffer;
  primitive_set->sizeof_data = len * size;

  SL(vector_buffer
     (prim_set_lst[id].index_list, &len, NULL, NULL, &buffer));
  assert(primitive_set->data != NULL || len == 0);
  primitive_set->index_list = buffer;
  primitive_set->nb_indices = len;

  SL(vector_buffer
     (prim_set_lst[id].attrib_list, &len, NULL, NULL, &buffer));
  primitive_set->attrib_list = buffer;
  primitive_set->nb_attribs = len;

  primitive_set->primitive_type = prim_set_lst[id].primitive_type;

exit:
  return err;

error:
  goto exit;
}

