#ifndef SYS_H
#define SYS_H

#ifndef __GNUC__
  #error "Unsupported compiler."
#endif

#define EXPORT_SYM __attribute__((visibility("default")))
#define HIDE_SYM __attribute__((visibility("hidden")))
#define FINLINE inline __attribute__((always_inline))
#define NOINLINE __attribute__((noinline))
#define ALIGN(sz) __attribute__((aligned(sz)))
#define ASSERT(x) assert(x)
#define CHOOSE_EXPR(cond, vtrue, vfalse) \
    __builtin_choose_expr(cond, vtrue, vfalse)

#endif /* SYS_H */
