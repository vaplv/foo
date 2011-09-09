#ifndef SYS_H
#define SYS_H

#include <stdint.h>

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

#define IS_ALIGNED(addr, alignment) \
  (((uintptr_t)(addr) & ((alignment)-1)) == 0)

#define UNUSED \
  __attribute__((unused))

#define CHOOSE_EXPR(cond, vtrue, vfalse) \
  __builtin_choose_expr(cond, vtrue, vfalse)

#define STATIC_ASSERT(condition, msg) \
  extern char STATIC_ASSERT_##msg[1 -  2*(!(condition))] UNUSED

#endif /* SYS_H */

