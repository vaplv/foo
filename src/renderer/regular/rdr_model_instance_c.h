#ifndef RDR_MODEL_INSTANCE_C_H
#define RDR_MODEL_INSTANCE_C_H

#include "renderer/rdr_error.h"
#include <stddef.h>

struct aosf44;
struct rdr_draw_desc;
struct rdr_model_instance;
struct rdr_system;

LOCAL_SYM enum rdr_error
rdr_draw_instances
  (struct rdr_system* sys,
   const struct aosf44* view_matrix,
   const struct aosf44* proj_matrix,
   size_t nb_instances,
   struct rdr_model_instance** instance_list,
   const struct rdr_draw_desc* desc);

#endif /* RDR_MODEL_INSTANCE_C_H */

