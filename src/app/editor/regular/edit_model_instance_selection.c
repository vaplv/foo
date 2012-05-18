#include "app/core/app.h"
#include "app/core/app_model_instance.h"
#include "app/editor/regular/edit_context_c.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_model_instance_selection.h"
#include "app/editor/edit_context.h"
#include "stdlib/sl.h"
#include "stdlib/sl_hash_table.h"
#include "stdlib/sl_pair.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static size_t
hash_str(const void* key)
{
  const char* str = *(const char**)key;
  return sl_hash(str, strlen(str));
}

static bool
eq_str(const void* key0, const void* key1)
{
  const char* str0 = *(const char**)key0;
  const char* str1 = *(const char**)key1;
  return strcmp(str0, str1) == 0;
}

static void
on_destroy_model_instance(struct app_model_instance* instance, void*data)
{
  struct edit_context* ctxt = data;
  const char* name = NULL;
  bool b = false;

  assert(instance && data);
  APP(model_instance_name(instance, &name));
  EDIT(is_model_instance_selected(ctxt, name, &b));
  if(b) {
    EDIT(unselect_model_instance(ctxt, name));
  }
}

/*******************************************************************************
 *
 * Model instance selection functions.
 *
 ******************************************************************************/
enum edit_error
edit_init_model_instance_selection(struct edit_context* ctxt)
{
  enum app_error app_err = APP_NO_ERROR;
  enum edit_error edit_err = EDIT_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(UNLIKELY(!ctxt)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  sl_err = sl_create_hash_table
    (sizeof(char*),
     ALIGNOF(char*),
     sizeof(struct app_model_instance*),
     ALIGNOF(struct app_model_instance*),
     hash_str,
     eq_str,
     ctxt->allocator,
     &ctxt->selected_model_instance_htbl);
  if(sl_err != SL_NO_ERROR) {
    edit_err = sl_to_edit_error(sl_err);
    goto error;
  }
  app_err = app_attach_callback
    (ctxt->app,
     APP_SIGNAL_DESTROY_MODEL_INSTANCE,
     APP_CALLBACK(on_destroy_model_instance),
     ctxt);
  if(app_err != APP_NO_ERROR) {
    edit_err = app_to_edit_error(app_err);
    goto error;
  }

exit:
  return edit_err;
error:
  if(ctxt)
    EDIT(shutdown_model_instance_selection(ctxt));
  goto exit;
}

enum edit_error
edit_shutdown_model_instance_selection(struct edit_context* ctxt)
{
  bool b = false;

  if(UNLIKELY(!ctxt))
    return EDIT_INVALID_ARGUMENT;

  if(ctxt->selected_model_instance_htbl) {
    EDIT(clear_model_instance_selection(ctxt));
    SL(free_hash_table(ctxt->selected_model_instance_htbl));
  }
  APP(is_callback_attached
    (ctxt->app,
     APP_SIGNAL_DESTROY_MODEL_INSTANCE,
     APP_CALLBACK(on_destroy_model_instance),
     ctxt,
     &b));
  if(b) {
    APP(detach_callback
      (ctxt->app,
       APP_SIGNAL_DESTROY_MODEL_INSTANCE,
       APP_CALLBACK(on_destroy_model_instance),
       ctxt));
  }
  return EDIT_NO_ERROR;
}

enum edit_error
edit_select_model_instance(struct edit_context* ctxt, const char* instance_name)
{
  struct app_model_instance* instance = NULL;
  char* name = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(UNLIKELY(!ctxt || !instance_name)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  APP(get_model_instance(ctxt->app, instance_name, &instance));
  if(instance == NULL) {
    APP(log(ctxt->app, APP_LOG_ERROR,
      "the instance `%s' does not exist\n", instance_name));
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  name = MEM_CALLOC(ctxt->allocator, strlen(instance_name) + 1, sizeof(char));
  if(!name) {
    edit_err = EDIT_MEMORY_ERROR;
    goto error;
  }
  name = strcpy(name, instance_name);

  sl_err = sl_hash_table_insert
    (ctxt->selected_model_instance_htbl, &name, &instance);
  if(sl_err != SL_NO_ERROR) {
    edit_err = sl_to_edit_error(sl_err);
    goto error;
  }

exit:
  return edit_err;
error:
  if(name)
    MEM_FREE(ctxt->allocator, name);
  goto exit;
}

enum edit_error
edit_unselect_model_instance
  (struct edit_context* ctxt,
   const char* instance_name)
{
  char* name = NULL;
  enum edit_error edit_err = EDIT_NO_ERROR;
  struct sl_pair pair = { NULL, NULL };
  size_t i = 0;

  if(UNLIKELY(!ctxt || !instance_name)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }
  SL(hash_table_find_pair
    (ctxt->selected_model_instance_htbl, &instance_name, &pair));
  if(SL_IS_PAIR_VALID(&pair) == false) {
    APP(log(ctxt->app, APP_LOG_ERROR,
      "the instance `%s' does not exist\n", instance_name));
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }
  name = *(char**)pair.key;
  SL(hash_table_erase(ctxt->selected_model_instance_htbl, &instance_name, &i));
  assert(i == 1);
  MEM_FREE(ctxt->allocator, name);

exit:
  return edit_err;
error:
  goto exit;
}

enum edit_error
edit_is_model_instance_selected
  (struct edit_context* ctxt,
   const char* instance_name,
   bool* is_selected)
{
  void* ptr = NULL;

  if(UNLIKELY(!ctxt || !instance_name || !is_selected))
    return EDIT_INVALID_ARGUMENT;
  SL(hash_table_find(ctxt->selected_model_instance_htbl, &instance_name, &ptr));
  *is_selected = (ptr != NULL);
  return EDIT_NO_ERROR;
}

enum edit_error
edit_clear_model_instance_selection(struct edit_context* ctxt)
{
  struct sl_hash_table_it it;
  bool b = false;

  if(UNLIKELY(!ctxt))
    return EDIT_INVALID_ARGUMENT;

  SL(hash_table_begin(ctxt->selected_model_instance_htbl, &it, &b));
  while(b == false) {
    MEM_FREE(ctxt->allocator, *(char**)it.pair.key);
    SL(hash_table_it_next(&it, &b));
  }
  SL(hash_table_clear(ctxt->selected_model_instance_htbl));
  return EDIT_NO_ERROR;
}
