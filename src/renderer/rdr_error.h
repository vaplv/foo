#ifndef RDR_ERROR_H
#define RDR_ERROR_H

struct rdr_system;

enum rdr_error {
  RDR_DRIVER_ERROR,
  RDR_INTERNAL_ERROR,
  RDR_INVALID_ARGUMENT,
  RDR_MEMORY_ERROR,
  RDR_NO_ERROR,
  RDR_OVERFLOW_ERROR,
  RDR_REF_COUNT_ERROR,
  RDR_UNKNOWN_ERROR
};

#endif /* RDR_ERROR_H. */

