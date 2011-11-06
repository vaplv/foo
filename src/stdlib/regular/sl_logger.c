#include "stdlib/regular/sl.h"
#include "stdlib/sl_logger.h"
#include "stdlib/sl_sorted_vector.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct sl_logger {
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
  struct sl_log_stream* stream0 = (struct sl_log_stream*)a;
  struct sl_log_stream* stream1 = (struct sl_log_stream*)b;

  const uintptr_t p0[2] = {(uintptr_t)stream0->func, (uintptr_t)stream0->func};
  const uintptr_t p1[2] = {(uintptr_t)stream1->func, (uintptr_t)stream1->func};

  const int inf = (p0[0] < p1[0]) | ((p0[0] == p1[0]) & (p0[1] < p1[1]));
  const int sup = (p0[0] > p1[0]) | ((p0[0] == p1[0]) & (p0[1] > p1[1]));
  return -(inf) | (sup);
}

/*******************************************************************************
 *
 * Logger implementation.
 *
 ******************************************************************************/
EXPORT_SYM enum sl_error
sl_create_logger(struct sl_logger** out_logger)
{
  struct sl_logger* logger = NULL;
  enum sl_error sl_err = SL_NO_ERROR;
  
  if(!out_logger) {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }
  logger = calloc(1, sizeof(struct sl_logger));
  if(!logger) {
    sl_err = SL_MEMORY_ERROR;
    goto error;
  }
  sl_err = sl_create_sorted_vector
    (sizeof(struct sl_log_stream),
     ALIGNOF(struct sl_log_stream),
     compare_log_stream,
     &logger->stream_list);
  if(sl_err != SL_NO_ERROR)
    goto error;

  logger->buffer_len = BUFSIZ;
  logger->buffer = malloc(sizeof(char) * BUFSIZ);
  if(!logger->buffer) {
    sl_err = SL_MEMORY_ERROR;
    goto error;
  }
  logger->buffer[0] = '\n';

exit:
  if(out_logger)
    *out_logger = logger;
  return sl_err;

error:
  if(logger) {
    sl_err = sl_free_logger(logger);
    assert(sl_err == SL_NO_ERROR);
    logger = NULL;
  }
  goto exit;
}

EXPORT_SYM enum sl_error
sl_free_logger(struct sl_logger* logger)
{
  enum sl_error sl_err = SL_NO_ERROR;

  if(!logger) {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }
  if(logger->stream_list) {
    sl_err = sl_free_sorted_vector(logger->stream_list);
    if(sl_err != SL_NO_ERROR)
      goto error;
  }
  free(logger->buffer);
  free(logger);

exit:
  return sl_err;;
error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_logger_add_stream
  (struct sl_logger* logger,
   struct sl_log_stream* stream)
{
  enum sl_error sl_err = SL_NO_ERROR;

  if(!logger || !stream || !stream->func) {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_sorted_vector_insert(logger->stream_list, stream);
  if(sl_err != SL_NO_ERROR)
    goto error;

exit:
  return sl_err;
error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_clear_logger(struct sl_logger* logger)
{
  enum sl_error sl_err = SL_NO_ERROR;

  if(!logger) {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_clear_sorted_vector(logger->stream_list);
  if(sl_err != SL_NO_ERROR)
    goto error;

exit:
  return sl_err;

error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_logger_remove_stream
  (struct sl_logger* logger,
   struct sl_log_stream* stream)
{
  enum sl_error sl_err = SL_NO_ERROR;

  if(!logger || !stream) {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_sorted_vector_remove(logger->stream_list, stream);
  if(sl_err != SL_NO_ERROR)
    goto error;

exit:
  return sl_err;
error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_logger_print
  (struct sl_logger* logger,
   const char* fmt, 
   ...)
{
  va_list vargs_list;
  void* buffer = NULL;
  size_t len = 0;
  size_t stream_id = 0;
  enum sl_error sl_err = SL_NO_ERROR;
  int i = 0;
  bool is_va_start = false;

  if(!logger || !fmt || fmt[0] == '\0') {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }
  va_start(vargs_list, fmt);
  is_va_start = true;
 
  i = vsnprintf(logger->buffer, logger->buffer_len, fmt, vargs_list);
  assert(i > 0);
  if((size_t)i >= logger->buffer_len) {
    len = i + 1; /* +1 <=> null terminated character. */
    buffer = malloc(len * sizeof(char));
    if(!buffer) {
      sl_err = SL_MEMORY_ERROR;
      goto error;
    }
    free(logger->buffer);
    logger->buffer = buffer;
    logger->buffer_len = len;
    i = vsnprintf(logger->buffer, logger->buffer_len, fmt, vargs_list);
    assert((size_t)i < logger->buffer_len);
  }

  sl_err = sl_sorted_vector_buffer
    (logger->stream_list, &len, NULL, NULL, &buffer);
  if(sl_err != SL_NO_ERROR)
    goto error;

  for(stream_id = 0; stream_id < len; ++stream_id) {
    struct sl_log_stream* stream = (struct sl_log_stream*)buffer + stream_id;
    stream->func(logger->buffer, stream->data);
  }

exit:
  if(is_va_start)
    va_end(vargs_list);
  return sl_err;
error:
  goto exit;
}

