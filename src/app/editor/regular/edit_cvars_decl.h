/* When a cvar is declared here with the EDIT_CVAR(name, desc) macro, the cvar
 * `name' is automatically added to the edit_context. Its registration and
 * un registration from the app is also automatically handle thanks to the X
 * macro mechanism. */

EDIT_CVAR
  (grid_ndiv, 
   APP_CVAR_INT_DESC(10, 0, 10000))

EDIT_CVAR
  (grid_nsubdiv, 
   APP_CVAR_INT_DESC(10, 0, 10000))

EDIT_CVAR
  (grid_size, 
   APP_CVAR_FLOAT_DESC(1000.f, 0.f, FLT_MAX))

EDIT_CVAR
  (mouse_sensitivity,
   APP_CVAR_FLOAT_DESC(1.f, 0.01f, 10.f))

EDIT_CVAR
  (pivot_color, 
   APP_CVAR_FLOAT3_DESC(1.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f))

EDIT_CVAR
  (pivot_size,
   APP_CVAR_FLOAT_DESC(0.1f, FLT_MIN, FLT_MAX))

EDIT_CVAR   
  (project_path,
   APP_CVAR_STRING_DESC(NULL, NULL))

EDIT_CVAR
  (show_grid,
   APP_CVAR_BOOL_DESC(true))

EDIT_CVAR
  (show_selection,
   APP_CVAR_BOOL_DESC(true))

