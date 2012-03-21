#ifndef RDR_MESH_C_H
#define RDR_MESH_C_H

#include "renderer/rdr_mesh.h"
#include <stdbool.h>

enum rdr_mesh_signal {
  RDR_MESH_SIGNAL_UPDATE_DATA,
  RDR_MESH_SIGNAL_UPDATE_INDICES,
  RDR_NB_MESH_SIGNALS
};

struct rdr_mesh_attrib_desc {
  size_t stride;
  size_t offset;
  enum rb_type type;
  enum rdr_attrib_usage usage;
};

extern enum rdr_error
rdr_get_mesh_attribs
  (struct rdr_mesh* mesh,
   size_t* nb_attribs,
   struct rdr_mesh_attrib_desc* attrib_list);

extern enum rdr_error
rdr_get_mesh_indexed_data
  (struct rdr_mesh* mesh,
   size_t* nb_indices,
   struct rb_buffer** indices,
   struct rb_buffer** data);

extern enum rdr_error
rdr_attach_mesh_callback
  (struct rdr_mesh* mesh,
   enum rdr_mesh_signal signal,
   void (*func)(struct rdr_mesh*, void* data),
   void* data);

extern enum rdr_error
rdr_detach_mesh_callback
  (struct rdr_mesh* mesh,
   enum rdr_mesh_signal signal,
   void (*func)(struct rdr_mesh*, void* data),
   void* data);

extern enum rdr_error
rdr_is_mesh_callback_attached
  (struct rdr_mesh* mesh,
   enum rdr_mesh_signal signal,
   void (*func)(struct rdr_mesh*, void* data),
   void* data,
   bool* is_attached);

#endif /* RDR_MESH_C_H */

