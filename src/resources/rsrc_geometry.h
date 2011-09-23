#ifndef RSRC_GEOMETRY_H
#define RSRC_GEOMETRY_H

#include "resources/rsrc_error.h"
#include <stddef.h>

struct rsrc_context;
struct rsrc_geometry;
struct rsrc_wavefront_obj;

enum rsrc_type {
  RSRC_FLOAT,
  RSRC_FLOAT2,
  RSRC_FLOAT3,
  RSRC_FLOAT4
};

enum rsrc_attrib_usage {
  RSRC_ATTRIB_POSITION,
  RSRC_ATTRIB_NORMAL,
  RSRC_ATTRIB_TEXCOORD
};

enum rsrc_primitive_type {
  RSRC_POINT,
  RSRC_LINE,
  RSRC_TRIANGLE
};

struct rsrc_attrib {
  enum rsrc_type type;
  enum rsrc_attrib_usage usage;
};

struct rsrc_primitive_set {
  struct rsrc_attrib* attrib_list;
  const void* data;
  const size_t* index_list;
  size_t nb_attribs;
  size_t nb_indices;
  size_t sizeof_data;
  enum rsrc_primitive_type primitive_type;
};

extern enum rsrc_error
rsrc_create_geometry
  (struct rsrc_context* ctxt,
   struct rsrc_geometry** out_geom);

extern enum rsrc_error
rsrc_free_geometry
  (struct rsrc_context* ctxt,
   struct rsrc_geometry* geom);

extern enum rsrc_error
rsrc_geometry_from_wavefront_obj
  (struct rsrc_context* ctxt,
   struct rsrc_geometry* geom,
   const struct rsrc_wavefront_obj* wobj);

extern enum rsrc_error
rsrc_get_primitive_set_count
  (struct rsrc_context* ctxt,
   const struct rsrc_geometry* geom,
   size_t* out_nb_prim_set);

extern enum rsrc_error
rsrc_get_primitive_set
  (struct rsrc_context* ctxt,
   const struct rsrc_geometry* geom,
   size_t prim_list_id,
   struct rsrc_primitive_set* desc);

#endif /* RSRC_GEOMETRY_H */

