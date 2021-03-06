#ifndef RDR_SYSTEM_H
#define RDR_SYSTEM_H

#include "renderer/rdr.h"
#include "renderer/rdr_error.h"

struct mem_allocator;
struct rdr_system;

RDR_API enum rdr_error 
rdr_create_system
  (const char* graphic_driver,
   struct mem_allocator* allocator, /* May be NULL. */
   struct rdr_system** out_sys);

RDR_API enum rdr_error 
rdr_system_ref_get
  (struct rdr_system* sys);

RDR_API enum rdr_error
rdr_system_ref_put
  (struct rdr_system* sys);

RDR_API enum rdr_error
rdr_system_attach_log_stream
  (struct rdr_system* sys,
   void (*stream_func)(const char*, void* stream_data),
   void* stream_data);

RDR_API enum rdr_error
rdr_system_detach_log_stream
  (struct rdr_system* sys,
   void (*stream_func)(const char*, void* stream_data),
   void* stream_data);

#endif /* RDR_SYSTEM_H */

