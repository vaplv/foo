#ifndef RDR_SYSTEM_H
#define RDR_SYSTEM_H

#include "renderer/rdr_error.h"

struct rdr_system;

extern enum rdr_error 
rdr_create_system
  (const char* graphic_driver,
   struct rdr_system** out_sys);

extern enum rdr_error 
rdr_free_system
  (struct rdr_system* sys);

#endif /* RDR_SYSTEM_H */

