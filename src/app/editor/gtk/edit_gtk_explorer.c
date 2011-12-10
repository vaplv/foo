#include "app/core/app.h"
#include "app/core/app_error.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "app/core/app_world.h"
#include "app/editor/gtk/edit_gtk_c.h"
#include "app/editor/gtk/edit_gtk_explorer.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <gtk/gtk.h>

/*******************************************************************************
 *
 * Callbacks
 *
 ******************************************************************************/
static void
on_add_model
  (GtkMenuItem* item,
   gpointer data) /* edit. */
{
  GtkFileChooserDialog* dialog = NULL;
  GtkFileFilter* filter = NULL;
  struct edit* edit = data;
  g_return_if_fail(NULL != item);
  g_return_if_fail(NULL != data);

  dialog = GTK_FILE_CHOOSER_DIALOG
    (gtk_file_chooser_dialog_new
     ("Open file",
      GTK_WINDOW(gtk_builder_get_object(edit->builder, "window_explorer")),
      GTK_FILE_CHOOSER_ACTION_OPEN,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
      NULL));
  g_return_if_fail(NULL != dialog);

  filter = gtk_file_filter_new();
  g_return_if_fail(NULL != filter);
  gtk_file_filter_set_name(filter, "*.obj");
  gtk_file_filter_add_pattern(filter, "*.obj");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    struct app_model* model = NULL;
    struct app_model_instance* instance = NULL;
    struct app_world* world = NULL;
    gchar* filename = NULL;

    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    /* TEMP. */
    APP(create_model(edit->app, filename, &model));
    APP(instantiate_model(edit->app, model, &instance));
    APP(get_main_world(edit->app, &world));
    APP(world_add_model_instances(world, 1, &instance));

    g_free(filename);
  }
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
on_row_activated
  (GtkTreeView* tree_view,
   GtkTreeIter* iter,
   GtkTreePath* path,
   gpointer data) /* struct edit. */
{
  g_return_if_fail(NULL != tree_view);
  g_return_if_fail(NULL != iter);
  g_return_if_fail(NULL != path);
  g_return_if_fail(NULL != data);
  /* TODO open the editor of the object stored into the activated row. */
}

static gboolean
on_button_pressed
  (GtkWidget* widget,
   GdkEvent* event,
   gpointer data UNUSED) /* struct edit. */
{
  GdkEventButton* button_evt = NULL;

  g_return_val_if_fail(NULL != widget, FALSE);
  g_return_val_if_fail(NULL != event, FALSE);
  g_return_val_if_fail(NULL != data, FALSE);
  g_return_val_if_fail(GTK_IS_TREE_VIEW(widget), FALSE);

  button_evt = (GdkEventButton*)event;
  if(3 == button_evt->button) { /* Right click. */
    struct edit* edit = (struct edit*)data;
    GtkMenu* menu = GTK_MENU
      (gtk_builder_get_object(edit->builder, "explorer_popup_menu"));

    gtk_menu_popup
      (menu, NULL, NULL, NULL, data, button_evt->button, button_evt->time);

    return TRUE;
  } else {
    return FALSE;
  }
}

/*******************************************************************************
 *
 * Helper functions
 *
 ******************************************************************************/
static void
add_model
  (struct app_model* model,
   void* data)
{
  const char* model_name = NULL;
  GtkListStore* list_store = NULL;
  GtkTreeIter iter;
  struct edit* edit = data;
  assert(model && data);

  APP(get_model_name(model, &model_name));
  if(!model_name)
    model_name = "Unknown";

  list_store = GTK_LIST_STORE
    (gtk_builder_get_object(edit->builder, "explorer_list_store"));
  gtk_list_store_append(list_store, &iter);
  gtk_list_store_set(list_store, &iter, 0, "Model", 1, model_name, -1);
}

static void
remove_model
  (struct app_model* mode UNUSED,
   void* data UNUSED)
{
}

static enum edit_error
connect_explorer_signals(struct edit* edit)
{
  GtkTreeView* tree_view = NULL;
  GtkMenuItem* menu_item = NULL;

  assert(edit);

  tree_view = GTK_TREE_VIEW
    (gtk_builder_get_object(edit->builder, "explorer_tree_view"));
  g_signal_connect
    (tree_view,
     "row-activated",
     G_CALLBACK(on_row_activated),
     edit);
  g_signal_connect
    (tree_view,
     "button_press_event",
     G_CALLBACK(on_button_pressed),
     edit);

  menu_item = GTK_MENU_ITEM
    (gtk_builder_get_object(edit->builder, "explorer_add_model"));
  g_signal_connect
    (menu_item,
     "activate",
     G_CALLBACK(on_add_model),
     edit);

  return EDIT_NO_ERROR;
}

static enum edit_error
populate_explorer_list(struct edit* edit)
{
  GtkListStore* list_store = NULL;
  struct app_model** model_list = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum edit_error edit_err = EDIT_NO_ERROR;
  size_t i = 0;
  size_t len = 0;

  assert(edit);

  list_store = GTK_LIST_STORE
    (gtk_builder_get_object(edit->builder, "explorer_list_store"));

  app_err = app_get_model_list(edit->app, &len, &model_list);
  if(app_err != APP_NO_ERROR) {
    edit_err = app_to_edit_error(app_err);
    goto error;
  }

  for(i = 0; i < len; ++i) {
    GtkTreeIter iter;
    const char* model_name = NULL;

    APP(get_model_name(model_list[i], &model_name));
    if(!model_name)
      model_name = "Unknown";

    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter, 0, "Model", 1, model_name, -1);
  }

exit:
  return edit_err;

error:
  goto exit;
}

/*******************************************************************************
 *
 * Explorer functions.
 *
 ******************************************************************************/
EXPORT_SYM enum edit_error
edit_init_explorer(struct edit* edit)
{
  enum app_error app_err = APP_NO_ERROR;
  enum edit_error edit_err = EDIT_NO_ERROR;
  bool is_add_cbk_connected = false;
  bool is_remove_cbk_connected = false;

  if(!edit || !edit->app) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  app_err = app_connect_model_callback
    (edit->app, APP_MODEL_EVENT_ADD, add_model, (void*)edit);
  if(app_err != APP_NO_ERROR) {
    edit_err = app_to_edit_error(app_err);
    goto error;
  }
  is_add_cbk_connected = true;
  app_err = app_connect_model_callback
    (edit->app, APP_MODEL_EVENT_REMOVE, remove_model, (void*)edit);
  if(app_err != APP_NO_ERROR) {
    edit_err = app_to_edit_error(app_err);
    goto error;
  }
  is_remove_cbk_connected = true;

  edit_err = connect_explorer_signals(edit);
  if(edit_err != EDIT_NO_ERROR)
    goto error;
  edit_err = populate_explorer_list(edit);
  if(edit_err != EDIT_NO_ERROR)
    goto error;

exit:
  return edit_err;

error:
  if(is_add_cbk_connected) {
    APP(disconnect_model_callback
      (edit->app, APP_MODEL_EVENT_ADD, add_model, (void*)edit));
  }
  if(is_remove_cbk_connected) {
    APP(disconnect_model_callback
      (edit->app, APP_MODEL_EVENT_REMOVE, remove_model, (void*)edit));
  }
  goto exit;
}

EXPORT_SYM enum edit_error
edit_shutdown_explorer(struct edit* edit)
{
  enum edit_error edit_err = EDIT_NO_ERROR;
  bool b = false;

  if(!edit) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  if(edit->app) {
    APP(is_model_callback_connected
      (edit->app, APP_MODEL_EVENT_ADD, add_model, (void*) edit, &b));
    if(true == b) {
      APP(disconnect_model_callback
        (edit->app, APP_MODEL_EVENT_ADD, add_model, (void*)edit));
    }
    APP(is_model_callback_connected
      (edit->app, APP_MODEL_EVENT_REMOVE, add_model, (void*) edit, &b));
    if(true == b) {
      APP(disconnect_model_callback
        (edit->app, APP_MODEL_EVENT_REMOVE, remove_model, (void*)edit));
    }
  }

exit:
  return edit_err;
error:
  goto exit;
}

