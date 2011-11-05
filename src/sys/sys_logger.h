#ifndef SYS_LOGGER_H
#define SYS_LOGGER_H

#include "sys/ref_count.h"
#include "sys/sys.h"
#include "sys/sys_error.h"

struct sys_log_stream {
  void (*func)(const char* msg, void* data);
  void* data;
};

REF_COUNT(struct sys_logger);

extern enum sys_error
sys_create_logger
  (struct sys* sys,
   struct sys_logger** out_logger);

extern enum sys_error
sys_free_logger
  (struct sys* sys,
   struct sys_logger* sys_logger);

extern enum sys_error
sys_logger_add_stream
  (struct sys* sys,
   struct sys_logger* logger,
   struct sys_log_stream* stream);

extern enum sys_error
sys_logger_remove_stream
  (struct sys* sys,
   struct sys_logger* logger,
   struct sys_log_stream* stream);

extern enum sys_error
sys_clear_logger
  (struct sys* sys,
   struct sys_logger* logger);

extern enum sys_error
sys_logger_print
  (struct sys* sys,
   struct sys_logger* logger,
   const char* fmt,
   ...) FORMAT_PRINTF(3, 4);

#endif /* LOG_H */
