#ifndef RB_H
#define RB_H

#include "render_backend/rb_types.h"
#include "sys/sys.h"

#ifndef NDEBUG
  #include <assert.h>
  #define RB(func) assert(0 == rb_##func)
#else
  #define RB(func) rb_##func
#endif

#if defined(BUILD_RB)
  #define RB_API EXPORT_SYM
#else
  #define RB_API IMPORT_SYM
#endif

/* Define the prototypes of the render backend functions. The generated
 * prototypes are prefixed by rb. The returned type is an integer. */
#define RB_FUNC(func_name, ...) \
  RB_API int rb_##func_name(__VA_ARGS__);

#include "render_backend/rb_func.h"

#undef RB_FUNC
#endif /* RB_H */

