#ifndef RDR_SYSTEM_C_H
#define RDR_SYSTEM_C_H

#include "render_backend/rbi.h"
#include "render_backend/rbu.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"

struct rb_context;

struct rdr_system {
  struct mem_allocator* allocator;
  struct ref ref;

  /* Render backend data. */
  struct rbi rb; 
  struct rb_context* ctxt;
  struct rb_config cfg;

  /* Render backend utils. */
  struct render_backend_utils {
    struct rbu_geometry quad;
    struct rbu_geometry circle;
  } rbu;
};

#endif /* RDR_SYSTEM_C_H. */

