#include "app/editor/gtk/edit_gtk_c.h"
#include "app/editor/gtk/edit_gtk_explorer.h"
#include "app/editor/edit.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <gtk/gtk.h>

/*******************************************************************************
 *
 * Editor functions.
 *
 ******************************************************************************/
EXPORT_SYM enum edit_error
edit_init
  (struct app* app,
   struct mem_allocator* specific_allocator, 
   struct edit** out_edit)
{
  struct edit* edit = NULL;
  struct mem_allocator* allocator = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;
  guint i = 0;
  gboolean b = TRUE;
  GtkWindow* window = NULL;

  if(!out_edit) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }
  allocator = specific_allocator ? specific_allocator : &mem_default_allocator;
  edit = MEM_CALLOC(allocator, 1, sizeof(struct edit));
  if(!edit) {
    edit_err = EDIT_MEMORY_ERROR;
    goto error;
  }
  edit->allocator = allocator;
  edit->app = app;

  b = gtk_init_check(0, NULL);
  if(!b) {
    edit_err = EDIT_INTERNAL_ERROR;
    goto error;
  }

  edit->builder = gtk_builder_new();
  i = gtk_builder_add_from_file
    (edit->builder, "./edit_gtk.glade", NULL);
  if(i == 0) {
    edit_err = EDIT_IO_ERROR;
    goto error;
  }
  window = GTK_WINDOW(gtk_builder_get_object(edit->builder, "window_explorer"));
  gtk_widget_show_all(GTK_WIDGET(window));

  edit_err = edit_init_explorer(edit);
  if(edit_err != EDIT_NO_ERROR)
    goto error;

exit:
  if(out_edit)
    *out_edit = edit;
  return edit_err;

error:
  if(edit) {
    EDIT(shutdown(edit));
    edit = NULL;
  }
  goto exit;
}

EXPORT_SYM enum edit_error
edit_shutdown(struct edit* edit)
{
  GtkWindow* window = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;

  if(!edit) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  edit_err = edit_shutdown_explorer(edit);
  if(edit_err != EDIT_NO_ERROR)
    goto error;

  window = GTK_WINDOW(gtk_builder_get_object(edit->builder, "window_explorer"));
  gtk_widget_destroy(GTK_WIDGET(window));
  MEM_FREE(edit->allocator, edit);

exit:
  return edit_err;
error:
  goto exit;
}

EXPORT_SYM enum edit_error
edit_run(struct edit* sys, bool* keep_running)
{
  if(!sys || !keep_running)
    return EDIT_INVALID_ARGUMENT;
  gtk_main_iteration_do(false);
  *keep_running = true;
  return EDIT_NO_ERROR;
}

