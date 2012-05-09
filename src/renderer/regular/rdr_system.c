#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_system.h"
#include "render_backend/rbi.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_system(struct ref* ref)
{
  struct rdr_system* sys = NULL;
  int err = 0;
  assert(NULL != ref);

  sys = CONTAINER_OF(ref, struct rdr_system, ref);

  if(LIKELY(sys->ctxt != NULL))
    RBI(&sys->rb, context_ref_put(sys->ctxt));
  if(LIKELY(sys->rbu.quad.rbi != NULL))
    RBU(geometry_ref_put(&sys->rbu.quad));
  if(LIKELY(sys->rbu.circle.rbi != NULL))
    RBU(geometry_ref_put(&sys->rbu.circle));

  err = rbi_shutdown(&sys->rb);
  assert(err == 0);

  MEM_FREE(sys->allocator, sys);
}

/*******************************************************************************
 *
 * Render system functions.
 *
 ******************************************************************************/
EXPORT_SYM enum rdr_error
rdr_create_system
  (const char* graphic_driver, 
   struct mem_allocator* specific_allocator,
   struct rdr_system** out_sys)
{
  struct mem_allocator* allocator = NULL;
  struct rdr_system* sys = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  if(!out_sys || !graphic_driver) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }

  allocator = specific_allocator ? specific_allocator : &mem_default_allocator;

  sys = MEM_CALLOC(allocator, 1, sizeof(struct rdr_system));
  if(!sys) {
    rdr_err = RDR_MEMORY_ERROR;
    goto error;
  }
  sys->allocator = allocator;
  ref_init(&sys->ref);

  #define CALL(func) \
    do { \
      if(0 != (err = func)) { \
        rdr_err = RDR_DRIVER_ERROR; \
        goto error; \
      } \
    } while(0)
  CALL(rbi_init(graphic_driver, &sys->rb));
  CALL(sys->rb.create_context(sys->allocator, &sys->ctxt));
  CALL(sys->rb.get_config(sys->ctxt, &sys->cfg));
  CALL(rbu_init_quad(&sys->rb, sys->ctxt, 0.f, 0.f, 1.f, 1.f, &sys->rbu.quad));
  CALL(rbu_init_circle
    (&sys->rb, sys->ctxt, 128, 0.f, 0.f, 1.f, &sys->rbu.circle));
  #undef CALL

exit:
  if(out_sys)
    *out_sys = sys;
  return rdr_err;

error:
  if(sys) {
    RDR(system_ref_put(sys));
    sys = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_system_ref_get(struct rdr_system* sys)
{
  if(UNLIKELY(!sys))
    return RDR_INVALID_ARGUMENT;
  ref_get(&sys->ref);
  return RDR_NO_ERROR;
}

EXPORT_SYM enum rdr_error
rdr_system_ref_put(struct rdr_system* sys)
{
  if(UNLIKELY(!sys))
    return RDR_INVALID_ARGUMENT;
  ref_put(&sys->ref, release_system);
  return RDR_NO_ERROR;
}

