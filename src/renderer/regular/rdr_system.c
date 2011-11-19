#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr_system.h"
#include "render_backend/rbi.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

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
  sys = MEM_CALLOC_I(allocator, 1, sizeof(struct rdr_system));
  if(!sys) {
    rdr_err = RDR_MEMORY_ERROR;
  }
  sys->allocator = allocator;

  err = rbi_init(graphic_driver, &sys->rb);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

  err = sys->rb.create_context(sys->allocator, &sys->ctxt);
  if(err != 0) {
    rdr_err = RDR_DRIVER_ERROR;
    goto error;
  }

exit:
  if(out_sys)
    *out_sys = sys;
  return rdr_err;

error:
  if(sys) {
    if(sys->ctxt) {
      err = sys->rb.free_context(sys->ctxt);
      assert(err == 0);
    }
    err = rbi_shutdown(&sys->rb);
    assert(err == 0);
    MEM_FREE_I(allocator, sys);
    sys = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_free_system(struct rdr_system* sys)
{
  struct mem_allocator* allocator = NULL;
  int err = 0;

  if(!sys)
    return RDR_INVALID_ARGUMENT;

  err = sys->rb.free_context(sys->ctxt);
  assert(err == 0);
  err = rbi_shutdown(&sys->rb);
  assert(err == 0);

  allocator = sys->allocator;
  MEM_FREE_I(allocator, sys);
  return RDR_NO_ERROR;
}

