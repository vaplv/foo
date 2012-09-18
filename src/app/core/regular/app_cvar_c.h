#ifndef APP_CVAR_C_H
#define APP_CVAR_C_H

#include "app/core/app_error.h"
#include "sys/sys.h"

struct app;
struct app_cvar;
struct mem_allocator;
struct sl_flat_map;

struct app_cvar_system {
  struct sl_flat_map* map; /* Associative container name -> cvar. */

  /* Builtin cvars. */
  #define APP_CVAR(name, desc) const struct app_cvar* name;
  #include "app/core/regular/app_cvars_decl.h"
  #undef APP_CVAR
};

LOCAL_SYM enum app_error
app_init_cvar_system
  (struct app* app);

LOCAL_SYM enum app_error
app_shutdown_cvar_system
  (struct app* app);

#endif /* APP_CVAR_C_H */

