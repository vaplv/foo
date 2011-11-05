#include "stdlib/sl_sorted_vector.h"
#include "sys/regular/sys_c.h"
#include "sys/sys.h"
#include "sys/sys_logger.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct logger {
  struct sys* sys;
  struct sl_sorted_vector* stream_list;
  char* buffer;
  size_t buffer_len;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static int
compare_log_stream(const void* a, const void* b)
{
  struct sys_log_stream* stream0 = (struct sys_log_stream*)a;
  struct sys_log_stream* stream1 = (struct sys_log_stream*)b;

  const uintptr_t p0[2] = {(uintptr_t)stream0->func, (uintptr_t)stream0->func};
  const uintptr_t p1[2] = {(uintptr_t)stream1->func, (uintptr_t)stream1->func};

  const int inf = (p0[0] < p1[0]) | ((p0[0] == p1[0]) & (p0[1] < p1[1]));
  const int sup = (p0[0] > p1[0]) | ((p0[0] == p1[0]) & (p0[1] > p1[1]));
  return -(inf) | (sup);
}

static void
free_logger(void* ctxt, void* data)
{
  struct sys* sys = ctxt;
  struct logger* logger = data;
  assert(sys && logger);

  if(logger->stream_list)
    SL(free_sorted_vector(sys_context(sys)->sl_ctxt, logger->stream_list));
  free(logger->buffer);

  RELEASE(sys);
}

/*******************************************************************************
 *
 * Logger implementation.
 *
 ******************************************************************************/
EXPORT_SYM enum sys_error
sys_create_logger(struct sys* sys, struct sys_logger** out_sys_logger)
{
  struct sys_logger* sys_logger = NULL;
  struct logger* logger = NULL;
  enum sys_error sys_err = SYS_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  
  if(!sys || !out_sys_logger) {
    sys_err = SYS_INVALID_ARGUMENT;
    goto error;
  }

  CREATE_REF_COUNT(sizeof(struct sys_logger), free_logger, sys_logger);
  if(!sys_logger) {
    sys_err = SYS_MEMORY_ERROR;
    goto error;
  }
  RETAIN(sys);
  sys_logger->ctxt = sys;

  logger = GET_REF_DATA(sys_logger);
  sl_err = sl_create_sorted_vector
    (sys_context(sys)->sl_ctxt,
     sizeof(struct sys_log_stream),
     ALIGNOF(struct sys_log_stream),
     compare_log_stream,
     &logger->stream_list);
  if(sl_err != SL_NO_ERROR) {
    sys_err = sl_to_sys_error(sl_err);
    goto error;
  }

  logger->buffer_len = BUFSIZ;
  logger->buffer = malloc(sizeof(char) * BUFSIZ);
  if(!logger->buffer) {
    sys_err = SYS_MEMORY_ERROR;
    goto error;
  }
  logger->buffer[0] = '\n';

exit:
  if(out_sys_logger)
    *out_sys_logger = sys_logger;
  return sys_err;

error:
  if(sys_logger) {
    while(RELEASE(sys_logger));
    sys_logger = NULL;
  }
  goto exit;
}

EXPORT_SYM enum sys_error
sys_free_logger(struct sys* sys, struct sys_logger* logger)
{
  if(!sys || !logger)
    return SYS_INVALID_ARGUMENT;

  RELEASE(logger);

  return SYS_NO_ERROR;
}

EXPORT_SYM enum sys_error
sys_logger_add_stream
  (struct sys* sys,
   struct sys_logger* sys_logger,
   struct sys_log_stream* stream)
{
  struct logger* logger = NULL;
  enum sys_error sys_err = SYS_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !sys_logger || !stream || !stream->func) {
    sys_err = SYS_INVALID_ARGUMENT;
    goto error;
  }
  logger = GET_REF_DATA(sys_logger);

  sl_err = sl_sorted_vector_insert
    (sys_context(sys)->sl_ctxt, logger->stream_list, stream);
  if(sl_err != SL_NO_ERROR) {
    sys_err = sl_to_sys_error(sl_err);
    goto error;
  }

exit:
  return sys_err;
error:
  goto exit;
}

EXPORT_SYM enum sys_error
sys_clear_logger(struct sys* sys, struct sys_logger* sys_logger)
{
  struct logger* logger = NULL;
  enum sl_error sl_err = SL_NO_ERROR;
  enum sys_error sys_err = SYS_NO_ERROR;

  if(!sys || !sys_logger) {
    sys_err = SYS_INVALID_ARGUMENT;
    goto error;
  }
  logger = GET_REF_DATA(sys_logger);

  sl_err = sl_clear_sorted_vector
    (sys_context(sys)->sl_ctxt, logger->stream_list);
  if(sl_err != SL_NO_ERROR) {
    sys_err = sl_to_sys_error(sl_err);
    goto error;
  }

exit:
  return sys_err;

error:
  goto exit;
}

EXPORT_SYM enum sys_error
sys_logger_remove_stream
  (struct sys* sys,
   struct sys_logger* sys_logger,
   struct sys_log_stream* stream)
{
  struct logger* logger = NULL;
  enum sys_error sys_err = SYS_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!sys || !sys_logger || !stream) {
    sys_err = SYS_INVALID_ARGUMENT;
    goto error;
  }
  logger = GET_REF_DATA(sys_logger);

  sl_err = sl_sorted_vector_remove
    (sys_context(sys)->sl_ctxt, logger->stream_list, stream);
  if(sl_err != SL_NO_ERROR) {
    sys_err = sl_to_sys_error(sl_err);
    goto error;
  }

exit:
  return sys_err;
error:
  goto exit;
}

EXPORT_SYM enum sys_error
sys_logger_print
  (struct sys* sys, 
   struct sys_logger* sys_logger,
   const char* fmt, 
   ...)
{
  va_list vargs_list;
  struct logger* logger = NULL;
  void* buffer = NULL;
  size_t len = 0;
  size_t stream_id = 0;
  enum sys_error sys_err = SYS_NO_ERROR;
  int i = 0;
  bool is_va_start = false;

  if(!sys || !sys_logger || !fmt || fmt[0] == '\0') {
    sys_err = SYS_INVALID_ARGUMENT;
    goto error;
  }
  logger = GET_REF_DATA(sys_logger);

  va_start(vargs_list, fmt);
  is_va_start = true;
 
  i = vsnprintf(logger->buffer, logger->buffer_len, fmt, vargs_list);
  assert(i > 0);
  if((size_t)i >= logger->buffer_len) {
    len = i + 1; /* +1 <=> null terminated character. */
    buffer = malloc(len * sizeof(char));
    if(!buffer) {
      sys_err = SYS_MEMORY_ERROR;
      goto error;
    }
    free(logger->buffer);
    logger->buffer = buffer;
    logger->buffer_len = len;
    i = vsnprintf(logger->buffer, logger->buffer_len, fmt, vargs_list);
    assert((size_t)i < logger->buffer_len);
  }
  SL(sorted_vector_buffer
     (sys_context(sys)->sl_ctxt, 
      logger->stream_list, 
      &len, 
      NULL, 
      NULL, 
      &buffer));
  for(stream_id = 0; stream_id < len; ++stream_id) {
    struct sys_log_stream* stream = (struct sys_log_stream*)buffer + stream_id;
    stream->func(logger->buffer, stream->data);
  }

exit:
  if(is_va_start)
    va_end(vargs_list);
  return sys_err;
error:
  goto exit;
}

