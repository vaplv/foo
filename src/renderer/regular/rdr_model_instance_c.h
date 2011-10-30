#ifndef RDR_MODEL_INSTANCE_C_H
#define RDR_MODEL_INSTANCE_C_H

#include "renderer/rdr_error.h"
#include "renderer/regular/rdr_object.h"
#include <stddef.h>

struct aosf44;
struct rdr_system;
RDR_OBJECT(struct rdr_model_instance);

extern enum rdr_error
rdr_draw_instances
  (struct rdr_system* sys,
   const struct aosf44* view_matrix,
   const struct aosf44* proj_matrix,
   size_t nb_instances,
   struct rdr_model_instance** instance_list);

#endif /* RDR_MODEL_INSTANCE_C_H */

