#include "resources/regular/rsrc_context_c.h"
#include "resources/regular/rsrc_error_c.h"
#include "resources/regular/rsrc_wavefront_obj_c.h"
#include "resources/rsrc_context.h"
#include "resources/rsrc_geometry.h"
#include "stdlib/sl.h"
#include "stdlib/sl_hash_table.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
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
  struct ref ref;
  struct rsrc_context* ctxt;
  struct sl_vector* primitive_set_list; /* vector of vector of primitive_set. */
  struct sl_hash_table* hash_table; /* Internal hash table. */
};

/*******************************************************************************
 *
 * Vertex pair data type.
 *
 ******************************************************************************/
struct vvtvn {
  size_t v;
  size_t vt;
  size_t vn;
};

static size_t
hash(const void* key)
{
  return sl_hash(key, sizeof(struct vvtvn));
}

static bool
compare(const void* p0, const void* p1)
{
  const struct vvtvn* a = (const struct vvtvn*)p0;
  const struct vvtvn* b = (const struct vvtvn*)p1;
  return (a->v == b->v) & (a->vt == b->vt) & (a->vn == b->vn);
} 

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
/* Triangulate the face defined by nb_verts stored into vert_list. The
 * triangulation is performed without adding any new vertex by reindexing
 * the vert_list into out_vert_id_list. */
static enum rsrc_error
triangulate
  (struct rsrc_wavefront_obj_face* vert_list UNUSED, 
   size_t nb_verts,
   size_t max_nb_verts,
   size_t* out_nb_vert_ids,
   size_t* out_vert_id_list)
{
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  assert
    (  (!nb_verts || vert_list)
    && max_nb_verts >= 3
    && out_nb_vert_ids 
    && out_vert_id_list);

  /* Right now only triangles (sic!) and quads are supported. 
   * TODO implement a generic triangulation algorithm. */
  if(nb_verts != 3 && nb_verts != 4) {
    rsrc_err = RSRC_PARSING_ERROR;
    goto error;
  }
  if(nb_verts == 3) {
    *out_nb_vert_ids = 3;
    out_vert_id_list[0] = 0;
    out_vert_id_list[1] = 1;
    out_vert_id_list[2] = 2;
  } else {
    if(max_nb_verts < 6) {
      rsrc_err = RSRC_MEMORY_ERROR;
      goto error;
    }
    *out_nb_vert_ids = 6;
    out_vert_id_list[0] = 0;
    out_vert_id_list[1] = 1;
    out_vert_id_list[2] = 2;
    out_vert_id_list[3] = 0;
    out_vert_id_list[4] = 2;
    out_vert_id_list[5] = 3;
  }
exit:
  return rsrc_err;
error:
  goto exit;
}

static enum rsrc_error
build_triangle_list
  (struct rsrc_context* ctxt,
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
  #define MAX_TRIANGULATE_FACE_IDS 6
  size_t triangulate_face_ids[MAX_TRIANGULATE_FACE_IDS];
  struct sl_vector* data = NULL;
  struct sl_vector* indices = NULL;
  struct sl_vector* attribs = NULL;
  size_t face_id = 0;
  unsigned int nb_indices = 0;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  assert(ctxt
      && pos
      /*&& nor May be NULL.*/
      /*&& tex  May be NULL. */
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
    size_t nb_verts = 0, vert_id = 0, nb_vert_ids = 0;

    SL_FUNC(vector_buffer
       (faces[face_id],
        &nb_verts,
        NULL,
        NULL,
        (void**)&face_verts));

    err = triangulate
      (face_verts, 
       nb_verts, 
       MAX_TRIANGULATE_FACE_IDS, 
       &nb_vert_ids, 
       triangulate_face_ids);
    if(err != RSRC_NO_ERROR)
      goto error;

    for(vert_id = 0; vert_id < nb_vert_ids; ++vert_id) {
      unsigned int* index = NULL;
      const struct vvtvn key = {
        .v = face_verts[triangulate_face_ids[vert_id]].v,
        .vt = face_verts[triangulate_face_ids[vert_id]].vt,
        .vn = face_verts[triangulate_face_ids[vert_id]].vn
      };

      SL_FUNC(hash_table_find(tbl, (const void*)&key, (void**)&index));
      if(index != NULL) {
        SL_FUNC(vector_push_back(indices, (void*)index));
      } else {
        float face_vertex[8];
        memset(&face_vertex, 0, sizeof(face_vertex));

        /* NOTE: the obj indexing starts at 1. */
        if(key.v > 0)
          memcpy(face_vertex + 0, pos[key.v - 1], sizeof(float[3]));
        if(key.vn > 0)
          memcpy(face_vertex + 3, nor[key.vn - 1], sizeof(float[3]));
        if(key.vt > 0) {
          if(tex[key.vt - 1][2] != 0.f 
          && tex[key.vt - 1][2] != 1.f) {  /* We expect 2d tex coords. */
            err = RSRC_PARSING_ERROR;
            goto error;
          }
          memcpy(face_vertex + 6, tex[key.vt - 1], sizeof(float[2]));
        }

        SL_FUNC(vector_push_back(data, (void*)&face_vertex));
        SL_FUNC(vector_push_back(indices, (void*)&nb_indices));
        SL_FUNC(hash_table_insert
          (tbl, (struct vvtvn[]){key}, (unsigned int[]){nb_indices}));

        if(nb_indices == UINT_MAX) {
          err = RSRC_OVERFOW_ERROR;
          goto error;
        }
        ++nb_indices;
      }
    }
  }
  #undef SL_FUNC
  #undef MAX_TRIANGULATE_FACE_IDS

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

static void
release_geometry(struct ref* ref)
{
  struct rsrc_context* ctxt = NULL;
  struct rsrc_geometry* geom = NULL;
  assert(ref);

  geom = CONTAINER_OF(ref, struct rsrc_geometry, ref);
  ctxt = geom->ctxt;

  RSRC(clear_geometry(geom));

  if(geom->primitive_set_list) 
    SL(free_vector(geom->primitive_set_list));
  if(geom->hash_table)
    SL(free_hash_table(geom->hash_table));

  MEM_FREE(ctxt->allocator, geom);
  RSRC(context_ref_put(ctxt));
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
  geom = MEM_CALLOC(ctxt->allocator, 1, sizeof(struct rsrc_geometry));
  if(!geom) {
    err = RSRC_MEMORY_ERROR;
    goto error;
  }
  ref_init(&geom->ref);
  geom->ctxt = ctxt;
  RSRC(context_ref_get(ctxt));

  sl_err = sl_create_vector
    (sizeof(struct primitive_set),
     ALIGNOF(struct primitive_set),
     geom->ctxt->allocator,
     &geom->primitive_set_list);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }

  sl_err = sl_create_hash_table
    (sizeof(struct vvtvn),
     ALIGNOF(struct vvtvn),
     sizeof(unsigned int),
     ALIGNOF(unsigned int),
     hash,
     compare,
     geom->ctxt->allocator,
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
    MEM_FREE(geom->ctxt->allocator, geom);
    geom = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_geometry_ref_get(struct rsrc_geometry* geom)
{
  if(!geom)
    return RSRC_INVALID_ARGUMENT;
  ref_get(&geom->ref);
  return RSRC_NO_ERROR;
}

EXPORT_SYM enum rsrc_error
rsrc_geometry_ref_put(struct rsrc_geometry* geom)
{
  if(!geom)
    return RSRC_INVALID_ARGUMENT;
  ref_put(&geom->ref, release_geometry);
  return RSRC_NO_ERROR;
}

EXPORT_SYM enum rsrc_error
rsrc_clear_geometry(struct rsrc_geometry* geom)
{
  struct primitive_set* prim_set = NULL;
  size_t len = 0;
  size_t i = 0;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!geom) {
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
  (struct rsrc_geometry* geom,
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

  if(!geom || !wobj) {
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

  err = rsrc_clear_geometry(geom);
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
      (geom->ctxt,
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
    RSRC(clear_geometry(geom));
  }
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_get_primitive_set_count
  (const struct rsrc_geometry* geom,
   size_t* out_nb_prim_list)
{
  enum sl_error sl_err = SL_NO_ERROR;

  if(!geom || !out_nb_prim_list)
    return RSRC_INVALID_ARGUMENT;

  sl_err = sl_vector_length(geom->primitive_set_list, out_nb_prim_list);
  if(sl_err != SL_NO_ERROR)
    return sl_to_rsrc_error(sl_err);

  return RSRC_NO_ERROR;
}

EXPORT_SYM enum rsrc_error
rsrc_get_primitive_set
  (const struct rsrc_geometry* geom,
   size_t id,
   struct rsrc_primitive_set* primitive_set)
{
  struct primitive_set* prim_set_lst = NULL;
  void* buffer = NULL;
  size_t size = 0;
  size_t len = 0;
  enum rsrc_error err = RSRC_NO_ERROR;

  if(!geom || !primitive_set) {
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

