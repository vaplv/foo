#ifndef RDR_ERROR_H
#define RDR_ERROR_H

struct rdr_system;

enum rdr_error {
  RDR_DRIVER_ERROR,
  RDR_INTERNAL_ERROR,
  RDR_INVALID_ARGUMENT,
  RDR_NO_ERROR,
  RDR_MEMORY_ERROR,
  RDR_REF_COUNT_ERROR,
  RDR_UNKNOWN_ERROR
};

extern enum rdr_error
rdr_get_error_string
  (struct rdr_system* sys, 
   const char** str_err);

extern enum rdr_error
rdr_flush_error
  (struct rdr_system* sys);

#endif /* RDR_ERROR_H. */

