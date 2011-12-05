#ifndef REF_COUNT_H
#define REF_COUNT_H

#include "sys/sys.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#define ALIGN_REF_COUNT(structure, alignment) \
  structure { \
    void (*destructor)(void* ctxt, void* data); \
    void* ctxt; \
    unsigned int ref_count; \
    ALIGN(alignment) char data[]; \
  }

#define REF_COUNT(structure) \
  ALIGN_REF_COUNT(structure, 16)

#define RETAIN(o) \
  (assert(o), (o)->ref_count == UINT_MAX ? 0  : ++((o)->ref_count))

#define GET_REF_COUNT(o) \
  (assert(o), (const unsigned int)((o)->ref_count))

#define GET_REF_DATA(o) \
  (assert(o), (void*)&((o)->data))

#define RELEASE(o) \
  (assert((o) && (o)->ref_count != 0), \
   --((o)->ref_count), \
   (o)->ref_count == 0 \
   ? (o)->destructor((o)->ctxt, GET_REF_DATA(o)), free(o), (o = NULL), 0 \
   : (o)->ref_count)

#define CREATE_REF_COUNT(data_size, func, o) \
  (((o) = memalign \
    (ALIGNOF(REF_COUNT(struct), sizeof(REF_COUNT(struct)) + data_size)), \
   memset((o), 0, sizeof(REF_COUNT(struct))), \
   ((o) ? ((o)->destructor = func), RETAIN(o), (o) : NULL))

#endif /* REF_COUNT_H */
