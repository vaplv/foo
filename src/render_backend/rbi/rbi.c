#include "render_backend/rbi.h"
#include "sys/sys.h"
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>

static
void set_rbi_to_null(struct rbi* driver)
{
  if(driver)
    memset(driver, 0, sizeof(struct rbi));
}

EXPORT_SYM int
rbi_init(const char* library, struct rbi* driver)
{
  int err = 0;
  char* err_msg = NULL;

  if(!library || !driver)
    goto error;

  set_rbi_to_null(driver);

  driver->handle = dlopen(library, RTLD_LAZY | RTLD_LOCAL);

  if(!driver->handle) {
    err_msg = dlerror();
    goto error;
  }

  #define RB_FUNC(ret_type, func_name, ...)\
    driver->func_name = (ret_type (*)(__VA_ARGS__))(intptr_t) \
      dlsym(driver->handle, "rb_"#func_name); \
    if((err_msg=dlerror())) \
      goto error;

  #include "render_backend/rb_func.h"

  #undef RB_FUNC

exit:
  return err;

error:
  if(err_msg)
    fprintf(stderr, "%s\n", err_msg);

  rbi_shutdown(driver);

  err = -1;
  goto exit;
}

EXPORT_SYM int
rbi_shutdown(struct rbi* driver)
{
  if(!driver)
    return -1;

  if(driver->handle)
    dlclose(driver->handle);

  set_rbi_to_null(driver);
  return 0;
}

