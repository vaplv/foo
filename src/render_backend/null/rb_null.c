#include "render_backend/rb.h"
#include "sys/sys.h"
#include <string.h>

#ifdef __GNUC__
  #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

/* This definition only works for functons that do not return an aggregate
 * (struct, union.). */
#define RB_FUNC(ret_type, func_name, ...) \
  EXPORT_SYM ret_type \
  rb_##func_name(__VA_ARGS__) \
  { \
    return (ret_type)0; \
  }

#include "render_backend/rb_func.h"

#undef RB_FUNC
