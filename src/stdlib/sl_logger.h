#ifndef SL_LOGGER_H
#define SL_LOGGER_H

#include "stdlib/sl.h"
#include "stdlib/sl_error.h"
#include "sys/sys.h"
#include <stdarg.h>

struct sl_log_stream {
  void (*func)(const char* msg, void* data);
  void* data;
};

struct mem_allocator;
struct sl_logger;

SL_API enum sl_error
sl_create_logger
  (struct mem_allocator* allocator, /* May be NULL. */
   struct sl_logger** out_logger);

SL_API enum sl_error
sl_free_logger
  (struct sl_logger* sl_logger);

SL_API enum sl_error
sl_logger_add_stream
  (struct sl_logger* logger,
   struct sl_log_stream* stream);

SL_API enum sl_error
sl_logger_remove_stream
  (struct sl_logger* logger,
   struct sl_log_stream* stream);

SL_API enum sl_error
sl_clear_logger
  (struct sl_logger* logger);

SL_API enum sl_error
sl_logger_print
  (struct sl_logger* logger,
   const char* fmt,
   ...) FORMAT_PRINTF(2, 3);

SL_API enum sl_error
sl_logger_vprint
  (struct sl_logger* logger,
   const char* fmt,
   va_list list);

#endif /* SL_LOGGER_H */
