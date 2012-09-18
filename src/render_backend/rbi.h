#ifndef RBI_H
#define RBI_H

#include "render_backend/rb_types.h"

#if defined(RBI_BUILD_SHARED_LIBRARY)
  #define RBI_API EXPORT_SYM
#elif defined(RBI_USE_SHARED_LIBRARY)
  #define RBI_API IMPORT_SYM
#else
  #define RBI_API extern
#endif

/* Define the render backend interface data structure; list of function
 * pointers toward a given implementation of the render backed functions. */
struct rbi {
  #define RB_FUNC(func_name, ...) int (*func_name)(__VA_ARGS__);
  #include "render_backend/rb_func.h"
  #undef RB_FUNC

  /* For internal use only. */
  void* handle;
};

#ifndef NDEBUG
  #include <assert.h>
  #define RBI(rbi, func) assert(0 == (rbi)->func)
#else
  #define RBI(rbi, func) ((rbi)->func)
#endif

RBI_API int rbi_init(const char* library, struct rbi*);
RBI_API int rbi_shutdown(struct rbi*);

#endif /* RBI_H */

