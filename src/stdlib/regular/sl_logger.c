#include "stdlib/sl_logger.h"
#include "stdlib/sl_flat_set.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct sl_logger {
  struct sl_flat_set* stream_list;
  struct mem_allocator* allocator;
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

  const uintptr_t p0[2] = {(uintptr_t)stream0->func, (uintptr_t)stream0->data};
  const uintptr_t p1[2] = {(uintptr_t)stream1->func, (uintptr_t)stream1->data};

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
sl_create_logger
  (struct mem_allocator* specific_allocator,
   struct sl_logger** out_logger)
{
  struct mem_allocator* allocator = NULL;
  struct sl_logger* logger = NULL;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!out_logger) {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }
  allocator = specific_allocator ? specific_allocator : &mem_default_allocator;
  logger = MEM_CALLOC(allocator, 1, sizeof(struct sl_logger));
  if(!logger) {
    sl_err = SL_MEMORY_ERROR;
    goto error;
  }
  sl_err = sl_create_flat_set
    (sizeof(struct sl_log_stream),
     ALIGNOF(struct sl_log_stream),
     compare_log_stream,
     specific_allocator,
     &logger->stream_list);
  if(sl_err != SL_NO_ERROR)
    goto error;

  logger->allocator = allocator;
  logger->buffer_len = 256;
  logger->buffer = MEM_ALLOC(allocator, sizeof(char) * logger->buffer_len);
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
  struct mem_allocator* allocator = NULL;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!logger) {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }
  if(logger->stream_list) {
    sl_err = sl_free_flat_set(logger->stream_list);
    if(sl_err != SL_NO_ERROR)
      goto error;
  }
  allocator = logger->allocator;
  MEM_FREE(allocator, logger->buffer);
  MEM_FREE(allocator, logger);

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
  sl_err = sl_flat_set_insert(logger->stream_list, stream, NULL);
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
  sl_err = sl_clear_flat_set(logger->stream_list);
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
  sl_err = sl_flat_set_erase(logger->stream_list, stream, NULL);
  if(sl_err != SL_NO_ERROR)
    goto error;

exit:
  return sl_err;
error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_logger_print(struct sl_logger* logger, const char* fmt, ...)
{
  va_list vargs_list;
  enum sl_error sl_err = SL_NO_ERROR;

  va_start(vargs_list, fmt);
  sl_err = sl_logger_vprint(logger, fmt, vargs_list);
  va_end(vargs_list);
  return sl_err;
}

EXPORT_SYM enum sl_error
sl_logger_vprint(struct sl_logger* logger, const char* fmt, va_list list)
{
  void* stream_list = NULL;
  size_t nb_streams = 0;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!logger || !fmt || fmt[0] == '\0') {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }

  sl_err = sl_flat_set_buffer
    (logger->stream_list, &nb_streams, NULL, NULL, &stream_list);
  if(sl_err != SL_NO_ERROR)
    goto error;

  if(nb_streams > 0) {
    size_t stream_id = 0;
    va_list tmp_list;
    int i = 0;

    va_copy(tmp_list, list);
    i = vsnprintf(logger->buffer, logger->buffer_len, fmt, tmp_list);
    va_end(tmp_list);

    assert(i > 0);
    if((size_t)i >= logger->buffer_len) {
      const size_t new_len = i + 1; /* +1 <=> null terminated character. */
      void* buffer = MEM_ALLOC(logger->allocator, new_len * sizeof(char));
      if(!buffer) {
        sl_err = SL_MEMORY_ERROR;
        goto error;
      }
      MEM_FREE(logger->allocator, logger->buffer);
      logger->buffer = buffer;
      logger->buffer_len = new_len;
      va_copy(tmp_list, list);
      i = vsnprintf(logger->buffer, logger->buffer_len, fmt, tmp_list);
      va_end(tmp_list);
      assert((size_t)i < logger->buffer_len);
    }

    for(stream_id = 0; stream_id < nb_streams; ++stream_id) {
      const struct sl_log_stream* stream =
        (const struct sl_log_stream*)stream_list + stream_id;
      stream->func(logger->buffer, stream->data);
    }
  }
exit:
  return sl_err;
error:
  goto exit;
}

