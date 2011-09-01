/*******************************************************************************
 *
 * List of macros to declare and use a render object data structure.
 *
 ******************************************************************************/
#ifndef RDR_OBJECT_H
#define RDR_OBJECT_H

#include "renderer/rdr_error.h"
#include "sys/sys.h"
#include <limits.h>

struct rdr_system;
typedef void (*destructor_t)(struct rdr_system*, void* data);

/* Declares a render object data structure. Its internal data may be considered
 * as private data, i.e. they may be acceded only throw the accessor macros. 
 * By default, the render object data are aligned on 16 Bytes. */
#define RDR_OBJECT(structure) \
  structure { \
    unsigned int ref_count; \
    destructor_t destructor; \
    ALIGN(16) char data[]; \
  }

/* Accessor macros.
 *  - sys: pointer toward a struct rdr_system;
 *  - obj: pointer toward an instance of a render object. */
#define RDR_GET_OBJECT_REF_COUNT(sys, obj)(const unsigned int)((obj)->ref_count)
#define RDR_GET_OBJECT_DATA(sys, obj) (void*)&((obj)->data)

/* Increment the reference counter of a render object. Return RDR_NO_ERROR or
 * RDR_REF_COUNT_ERROR if the counter cannot can be incremented or not
 * respectively.
 *  - sys: pointer toward a struct rdr_system;
 *  - obj: pointer toward an instance of a render object. */
#define RDR_RETAIN_OBJECT(sys, obj) \
  (obj->ref_count == UINT_MAX \
    ? RDR_REF_COUNT_ERROR \
    : ++obj->ref_count, RDR_NO_ERROR)

/* Decrement the reference counter of a render object. Delete the object and
 * its associated data if the counter reaches 0. Return the new value of the
 * counter.
 *  - sys: pointer toward a struct rdr_system;
 *  - obj: pointer toward an instance of a render object. */
#define RDR_RELEASE_OBJECT(sys, obj) \
  (assert(obj->ref_count != 0), \
  --obj->ref_count, \
  obj->ref_count == 0 \
    ? obj->destructor(sys, RDR_GET_OBJECT_DATA(sys, obj)), free(obj), 0 \
    : obj->ref_count)

/* Create a render object instance.
 *  - sys: pointer toward a struct rdr_system;
 *  - data_size: size in bytes of the data of the render object;
 *  - func: destructor of the render object data;
 *  - obj: destination pointer of the created render object. */
#define RDR_CREATE_OBJECT(sys, data_size, func, obj) \
  ((obj = calloc(1, sizeof(RDR_OBJECT(struct)) + data_size)),\
   (obj \
     ? (obj->destructor = func), RDR_RETAIN_OBJECT(sys, obj), RDR_NO_ERROR \
     : RDR_MEMORY_ERROR)) \

#endif /* RDR_OBJECT_H */

