#ifndef SYS_H
#define SYS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifndef __GNUC__
  #error "Unsupported compiler."
#endif

#define EXPORT_SYM \
  __attribute__((visibility("default")))

#define HIDE_SYM \
  __attribute__((visibility("hidden")))

#define FINLINE \
  inline __attribute__((always_inline))

#define NOINLINE \
  __attribute__((noinline))

#define ALIGN(sz) \
  __attribute__((aligned(sz)))

#define ALIGNOF(type) \
  __alignof__(type)

#define ALIGN_SIZE(size, alignment) \
  (((size) + ((alignment) - 1)) & ~((alignment) - 1))

#define IS_ALIGNED(addr, alignment) \
  (((uintptr_t)(addr) & ((alignment)-1)) == 0)

#define BIGGEST_ALIGNMENT \
  __BIGGEST_ALIGNMENT__

#define UNUSED \
  __attribute__((unused))

#define CHOOSE_EXPR(cond, vtrue, vfalse) \
  __builtin_choose_expr(cond, vtrue, vfalse)

#define STATIC_ASSERT(condition, msg) \
  char STATIC_ASSERT_##msg[1 -  2*(!(condition))] UNUSED

#define FORMAT_PRINTF(a, b) \
  __attribute__((format(printf, a, b)))

#define FATAL(msg) \
  do { \
    fprintf(stderr, msg); \
    assert(0); \
    exit(-1); \
  } while(0) \

#define CONTAINER_OF(ptr, type, member) \
    (type*)((uintptr_t)ptr - offsetof(type, member))

#define IS_MEMORY_OVERLAPPED(d0, sz0, d1, sz1) \
  ((uintptr_t)(d0) >= (uintptr_t)(d1) && \
   (uintptr_t)(d0) < (uintptr_t)(d1) + (sz1)) || \
  ((uintptr_t)(d0) + (sz0) >= (uintptr_t)(d1) && \
   (uintptr_t)(d0) + (sz0) < (uintptr_t)(d1) + (sz1))

#define CONCAT__(a, b) a ## b
#define CONCAT(a, b) \
  CONCAT__(a, b)

#define LIKELY(x) \
  __builtin_expect(x, 1)

#define UNLIKELY(x) \
  __builtin_expect(x, 0)

#define STR__(x) #x
#define STR(x) STR__(x)
 
#endif /* SYS_H */

