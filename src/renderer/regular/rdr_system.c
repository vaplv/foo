#include "renderer/regular/rdr_error_c.h"
#include "renderer/regular/rdr_imdraw_c.h"
#include "renderer/regular/rdr_system_c.h"
#include "renderer/rdr.h"
#include "renderer/rdr_system.h"
#include "render_backend/rbi.h"
#include "stdlib/sl.h"
#include "stdlib/sl_logger.h"
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
  if(LIKELY(sys->rbu.solid_parallelepiped.rbi != NULL))
    RBU(geometry_ref_put(&sys->rbu.solid_parallelepiped));
  if(LIKELY(sys->rbu.wire_parallelepiped.rbi != NULL))
    RBU(geometry_ref_put(&sys->rbu.wire_parallelepiped));
  if(LIKELY(sys->rbu.cone.rbi != NULL))
    RBU(geometry_ref_put(&sys->rbu.cone));
  if(LIKELY(sys->rbu.cylinder.rbi != NULL))
    RBU(geometry_ref_put(&sys->rbu.cylinder));

  RDR(shutdown_im_rendering(sys));

  if(LIKELY(MEM_IS_ALLOCATOR_VALID(&sys->render_backend_allocator))) {
    if(0 != MEM_ALLOCATED_SIZE(&sys->render_backend_allocator)) {
      #define BUF_SIZE 2048
      char buffer[BUF_SIZE];
      const char* log_string = "render backend leaks summary:\n%s\n";

      MEM_DUMP(&sys->render_backend_allocator, buffer, BUF_SIZE);
      if(sys->logger) {
        SL(logger_print(sys->logger, log_string, buffer));
      } else {
        fprintf(stderr, log_string, buffer);
      }
      #undef BUF_SIZE
    }
    mem_shutdown_proxy_allocator(&sys->render_backend_allocator);
  }

  if(LIKELY(sys->logger != NULL))
    SL(free_logger(sys->logger));

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
  enum sl_error sl_err = SL_NO_ERROR;
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
  sys->next_model_id = 1; /* Zero is an invalid id. */

  /* The render backend does not use the user defined allocator since when its
   * interface is shut down the __FILE__ pointers reported by its internal
   * allocations are no more valid. Consequently the dump of the allocator may
   * be corrupted. We thus have to report the render backend memory leaks
   * *before* the shut down of the rbi.
   *
   * Note that he default allocator ensure that the __FILE__ information is not
   * used. We thus ensure the memory dump consistency by tracking the render
   * backend allocations with a proxy of the default allocator and by
   * reporting its memory leaks *before* the rbi shutdown. */
  mem_init_proxy_allocator
    ("render backend", &sys->render_backend_allocator, &mem_default_allocator);

  sl_err = sl_create_logger(sys->allocator, &sys->logger);
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }

  #define CALL(func) \
    do { \
      if(0 != (err = func)) { \
        rdr_err = RDR_DRIVER_ERROR; \
        goto error; \
      } \
    } while(0)
  CALL(rbi_init(graphic_driver, &sys->rb));
  CALL(sys->rb.create_context(&sys->render_backend_allocator, &sys->ctxt));
  CALL(sys->rb.get_config(sys->ctxt, &sys->cfg));

  /* Init utils geometries. */
  CALL(rbu_init_circle
    (&sys->rb, sys->ctxt,
     32,
     (float[]){0.f, 0.f},
     1.f,
     &sys->rbu.circle));
  CALL(rbu_init_cylinder
    (&sys->rb, sys->ctxt,
     32, /* nslices */
     0.5f, /* base_radius */
     0.f, /* top radius */
     1.f, /* height */
     (float[]){0.f, 0.f, 0.f},
     &sys->rbu.cone));
  CALL(rbu_init_cylinder
    (&sys->rb, sys->ctxt,
     32, /* nslices */
     0.5f, /* base_radius */
     0.5f, /* top radius */
     1.f, /* height */
     (float[]){0.f, 0.f, 0.f},
     &sys->rbu.cylinder));
  CALL(rbu_init_quad
    (&sys->rb,
     sys->ctxt,
     0.f, 0.f, 1.f, 1.f,
     &sys->rbu.quad));
  CALL(rbu_init_parallelepiped
    (&sys->rb, sys->ctxt,
     (float[]){0.f, 0.f, 0.f},
     (float[]){1.f, 1.f, 1.f},
     false,
     &sys->rbu.solid_parallelepiped));
  CALL(rbu_init_parallelepiped
    (&sys->rb, sys->ctxt,
     (float[]){0.f, 0.f, 0.f},
     (float[]){1.f, 1.f, 1.f},
     true,
     &sys->rbu.wire_parallelepiped));
  #undef CALL

  rdr_err = rdr_init_im_rendering(sys);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

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

EXPORT_SYM enum rdr_error
rdr_system_attach_log_stream
  (struct rdr_system* sys,
   void (*stream_func)(const char*, void* stream_data),
   void* stream_data)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(UNLIKELY(!sys || !stream_func)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_logger_add_stream
    (sys->logger, (struct sl_log_stream[]){{stream_func, stream_data}});
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }
exit:
  return rdr_err;
error:
  goto exit;
}

EXPORT_SYM enum rdr_error
rdr_system_detach_log_stream
  (struct rdr_system* sys,
   void (*stream_func)(const char*, void* stream_data),
   void* stream_data)
{
  enum rdr_error rdr_err = RDR_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(UNLIKELY(!sys || !stream_func)) {
    rdr_err = RDR_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_logger_remove_stream
    (sys->logger, (struct sl_log_stream[]){{stream_func, stream_data}});
  if(sl_err != SL_NO_ERROR) {
    rdr_err = sl_to_rdr_error(sl_err);
    goto error;
  }
exit:
  return rdr_err;
error:
  goto exit;
}

/*******************************************************************************
 *
 * Private render system functions.
 *
 ******************************************************************************/
enum rdr_error
rdr_gen_model_id(struct rdr_system* sys, uint16_t* out_id)
{
  if(UNLIKELY(!sys || !out_id))
    return RDR_INVALID_ARGUMENT;
  if(UNLIKELY(sys->next_model_id == 0))
    return RDR_MEMORY_ERROR;
  *out_id = sys->next_model_id++;
  return RDR_NO_ERROR;
}

enum rdr_error
rdr_delete_model_id(struct rdr_system* sys, uint16_t id)
{
  if(UNLIKELY
  (  !sys 
  || id >= sys->next_model_id 
  || (id == UINT16_MAX && sys->next_model_id != 0)))
    return RDR_INVALID_ARGUMENT;
  /* The id is released only if it is equal to the next id minus one. In all
   * other cases, we can't release the id. */
  sys->next_model_id -= (id == sys->next_model_id - 1);
  return RDR_NO_ERROR;
}

