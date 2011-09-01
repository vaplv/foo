#ifndef RDR_MESH_H
#define RDR_MESH_H

#include "renderer/rdr_attrib.h"
#include "renderer/rdr_error.h"
#include <stddef.h>

struct rdr_mesh_attrib {
  enum rdr_attrib_usage usage;
  enum rdr_type type;
};

struct rdr_system;
struct rdr_mesh;

extern enum rdr_error
rdr_create_mesh
  (struct rdr_system* sys,
   struct rdr_mesh** out_mesh);

extern enum rdr_error
rdr_free_mesh
  (struct rdr_system* sys,
   struct rdr_mesh* mesh);

extern enum rdr_error
rdr_mesh_data
  (struct rdr_system* sys,
   struct rdr_mesh* mesh,
   size_t attribs_count,
   const struct rdr_mesh_attrib* list_of_attribs,
   size_t data_size,
   const void* data);

extern enum rdr_error
rdr_mesh_indices
  (struct rdr_system* sys,
   struct rdr_mesh* mesh,
   size_t nb_indices,
   const unsigned int* indices);

#endif /* RDR_MESH_H */

