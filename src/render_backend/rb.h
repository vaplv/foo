#ifndef RB_H
#define RB_H

#include "render_backend/rb_types.h"

#ifndef NDEBUG
  #include <assert.h>
  #define RB(func) assert(0 == rb_##func)
#else
  #define RB(func) rb_##func
#endif

/* Define the prototypes of the render backend functions. The generated
 * prototypes are prefixed by rb. */
#define RB_FUNC(ret_type, func_name, ...) \
  extern ret_type rb_##func_name(__VA_ARGS__);

#include "render_backend/rb_func.h"

#undef RB_FUNC
#endif /* RB_H */

