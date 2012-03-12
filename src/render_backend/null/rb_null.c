#include "render_backend/rb.h"
#include "sys/sys.h"
#include <string.h>

#ifdef __GNUC__
  #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

/* Define exportable function prototypes. */
#define RB_FUNC(ret_type, func_name, ...) \
  EXPORT_SYM ret_type \
  CONCAT(rb_, func_name)(__VA_ARGS__);

#include "render_backend/rb_func.h"

#undef RB_FUNC
  
/* Do not use the X macro to define the rb_get_config function body. */
static int
rb_get_config__(struct rb_context* ctxt, struct rb_config* cfg);
#define get_config get_config__

/* Define function body. This definition only works for functions that do not
 * return an aggregate (struct, union.). */
#define RB_FUNC(ret_type, func_name, ...) \
  ret_type \
  CONCAT(rb_, func_name)(__VA_ARGS__) \
  { \
    return (ret_type)0; \
  }

#include "render_backend/rb_func.h"

#undef RB_FUNC

/* Define the specific body of the rb_get_config config function. */
int
rb_get_config(struct rb_context* ctxt, struct rb_config* cfg)
{
  rb_get_config__(NULL, NULL); /* Avoid the `unused static function' warning. */
  cfg->max_tex_max_anisotropy = SIZE_MAX;
  cfg->max_tex_size = SIZE_MAX;
  return 0;
}

