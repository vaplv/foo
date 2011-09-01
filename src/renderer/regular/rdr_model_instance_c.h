#ifndef RDR_MODEL_INSTANCE_C_H
#define RDR_MODEL_INSTANCE_C_H

#include "renderer/rdr_error.h"
#include <stddef.h>

struct rdr_system;
struct rdr_model_instance;

extern enum rdr_error
rdr_draw_instances
  (struct rdr_system* sys,
   const float view_matrix[16],
   const float proj_matrix[16],
   size_t nb_instances,
   struct rdr_model_instance** instance_list);

#endif /* RDR_MODEL_INSTANCE_C_H */

