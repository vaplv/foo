#ifndef RDR_MESH_C_H
#define RDR_MESH_C_H

#include "renderer/regular/rdr_object.h"
#include "renderer/rdr_mesh.h"

struct rdr_mesh_callback;

struct rdr_mesh_callback_desc {
  void* data;
  enum rdr_error (*func)(struct rdr_system*, struct rdr_mesh*, void* data); 
};

struct rdr_mesh_attrib_desc {
  size_t stride;
  size_t offset;
  enum rb_type type;
  enum rdr_attrib_usage usage;
};

RDR_OBJECT(struct rdr_mesh);

extern enum rdr_error
rdr_get_mesh_attribs
  (struct rdr_system* sys,
   struct rdr_mesh* mesh,
   size_t* nb_attribs,
   struct rdr_mesh_attrib_desc* attrib_list);

extern enum rdr_error
rdr_get_mesh_indexed_data
  (struct rdr_system* sys,
   struct rdr_mesh* mesh,
   size_t* nb_indices,
   struct rb_buffer** indices,
   struct rb_buffer** data);

extern enum rdr_error
rdr_attach_mesh_callback
  (struct rdr_system* sys,
   struct rdr_mesh* mesh,
   const struct rdr_mesh_callback_desc* desc,
   struct rdr_mesh_callback** out_calback);

extern enum rdr_error
rdr_detach_mesh_callback
  (struct rdr_system* sys,
   struct rdr_mesh* mesh,
   struct rdr_mesh_callback* callback);

#endif /* RDR_MESH_C_H */

