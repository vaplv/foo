#ifndef RDR_SYSTEM_H
#define RDR_SYSTEM_H

#include "renderer/rdr_error.h"

struct mem_allocator;
struct rdr_system;

extern enum rdr_error 
rdr_create_system
  (const char* graphic_driver,
   struct mem_allocator* allocator, /* May be NULL. */
   struct rdr_system** out_sys);

extern enum rdr_error 
rdr_system_ref_get
  (struct rdr_system* sys);

extern enum rdr_error
rdr_system_ref_put
  (struct rdr_system* sys);

#endif /* RDR_SYSTEM_H */

