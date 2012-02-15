#ifndef RDR_SYSTEM_C_H
#define RDR_SYSTEM_C_H

#include "render_backend/rbi.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"

struct rb_context;

struct rdr_system {
  struct mem_allocator* allocator;
  struct ref ref;

  /* The render backend data. */
  struct rbi rb; 
  struct rb_context* ctxt;
  struct rb_config cfg;
};

#endif /* RDR_SYSTEM_C_H. */

