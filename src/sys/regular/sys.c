#include "stdlib/sl_context.h"
#include "sys/regular/sys_c.h"
#include "sys/sys.h"
#include <stdlib.h>

/*******************************************************************************
 *
 * Helper function.
 
 ******************************************************************************/
static void
free_sys(void* ctxt UNUSED, void* data) 
{
  struct sys_context* sys_ctxt = data;

  assert((uintptr_t)ctxt== 0xDEADBEEF && sys_ctxt);
  if(sys_ctxt->sl_ctxt)
    SL(free_context(sys_ctxt->sl_ctxt));
}

/*******************************************************************************
 *
 * Sys implementation.
 *
 ******************************************************************************/
EXPORT_SYM enum sys_error
sys_init(struct sys** out_sys)
{
  struct sys* sys = NULL;
  enum sys_error sys_err = SYS_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
 
  if(!out_sys) {
    sys_err = SYS_INVALID_ARGUMENT;
    goto error;
  }
  CREATE_REF_COUNT(sizeof(struct sys_context), free_sys, sys);
  STATIC_ASSERT(sizeof(void*) >= 4, Unexpected_pointer_size);
  sys->ctxt = (void*)0xDEADBEEF;

  if(!sys) {
    sys_err = SYS_MEMORY_ERROR;
    goto error;
  }
  sl_err = sl_create_context(&(sys_context(sys)->sl_ctxt));
  if(sl_err != SL_NO_ERROR) {
    sys_err = sl_to_sys_error(sl_err);
    goto error;
  }

exit:
  if(out_sys)
    *out_sys = sys;
  return sys_err;

error:
  if(sys) {
    while(RELEASE(sys));
    sys = NULL;
  }
  goto exit;
}

EXPORT_SYM enum sys_error
sys_shutdown(struct sys* sys)
{
  if(!sys) 
    return SYS_INVALID_ARGUMENT;
  RELEASE(sys);
  return SYS_NO_ERROR;
}

/*******************************************************************************
 *
 * Private function.
 *
 ******************************************************************************/
struct sys_context*
sys_context(struct sys* sys) 
{
  assert(sys);
  return (struct sys_context*)GET_REF_DATA(sys);
}

enum sys_error
sl_to_sys_error(enum sl_error sl_err)
{
  enum sys_error sys_err = SYS_UNKNOWN_ERROR;

  switch(sl_err) {
    case SL_ALIGNMENT_ERROR:
      sys_err = SYS_ALIGNMENT_ERROR;
      break;
    case SL_INVALID_ARGUMENT:
      sys_err = SYS_INVALID_ARGUMENT;
      break;
    case SL_MEMORY_ERROR:
      sys_err = SYS_MEMORY_ERROR;
      break;
    case SL_NO_ERROR:
      sys_err = SYS_NO_ERROR;
      break;
    default:
      sys_err = SYS_UNKNOWN_ERROR;
      break;
  }
  return sys_err;
}

