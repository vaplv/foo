#ifndef REF_COUNT_H
#define REF_COUNT_H

#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>

struct ref {
  int64_t ref_count;
};

static FINLINE void
ref_init(struct ref* ref)
{
  assert(NULL != ref);
  ref->ref_count = 1;
}

static FINLINE void
ref_get(struct ref* ref)
{
  assert(NULL != ref);
  ++ref->ref_count;
}

static FINLINE int
ref_put(struct ref* ref, void (*release)(struct ref*))
{
  assert(NULL != ref);
  assert(NULL != release);
  assert(0 != ref->ref_count);

  --ref->ref_count;
  if(0 == ref->ref_count) {
    release(ref);
    return 1;
  }
  return 0;
}

#endif /* REF_COUNT_H */
