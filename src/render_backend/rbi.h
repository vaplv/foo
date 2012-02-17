#ifndef RBI_H
#define RBI_H

#include "render_backend/rb_types.h"

/* Define the render backend interface data structure; list of function
 * pointers toward a given implementation of the render backed functions. */
struct rbi {
  #define RB_FUNC(ret_type, func_name, ...) ret_type (*func_name)(__VA_ARGS__);
  #include "render_backend/rb_func.h"
  #undef RB_FUNC

  /* For internal use only. */
  void* handle;
};

#ifndef NDEBUG
  #include <assert.h>
  #define RBI(rbi, func) assert(0 == (rbi).func)
#else
  #define RBI(rbi, func) (rbi).func
#endif

extern int rbi_init(const char* library, struct rbi*);
extern int rbi_shutdown(struct rbi*);

#endif /* RBI_H */

