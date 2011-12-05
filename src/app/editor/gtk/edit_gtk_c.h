#ifndef EDIT_GTK_C_H
#define EDIT_GTK_C_H

#include "app/core/app_error.h"
#include "app/editor/edit_error.h"
#include <gtk/gtk.h>

#ifndef NDEBUG
  #include <assert.h>
  #define APP(func) assert(APP_NO_ERROR == app_##func)
  #define EDIT(func) assert(EDIT_NO_ERROR == edit_##func)
#else
  #define APP(func) app_##func
  #define EDIT(func) edit_##func
#endif
 
struct mem_allocator;

struct edit {
  struct app* app;
  struct mem_allocator* allocator;
  GtkBuilder* builder;
};

extern enum edit_error
app_to_edit_error
  (enum app_error err);

#endif /* EDIT_GTK_C_H */

