#ifndef APP_CVAR_C_H
#define APP_CVAR_C_H

#include "app/core/app_error.h"

struct app;
struct app_cvar;
struct mem_allocator;
struct sl_flat_map;

struct app_cvar_system {
  struct sl_flat_map* map; /* Associative container name -> cvar. */
  /* Put the builtin cvar here. */
  #define APP_CVAR(name, desc) const struct app_cvar* name;
  #include "app/core/regular/app_cvars_decl.h"
  #undef APP_CVAR
};

extern enum app_error
app_init_cvar_system
  (struct app* app);

extern enum app_error
app_shutdown_cvar_system
  (struct app* app);

#endif /* APP_CVAR_C_H */

