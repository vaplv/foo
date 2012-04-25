#include "app/core/regular/app_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/regular/app_object.h"
#include "stdlib/sl.h"
#include "stdlib/sl_flat_map.h"
#include "stdlib/sl_string.h"
#include "sys/sys.h"
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static enum app_error
register_object(struct app* app, struct app_object* obj)
{
  const char* cstr = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  assert(obj && obj->name && app && obj->type != APP_NB_OBJECT_TYPES);

  SL(string_get(obj->name, &cstr));

  sl_err = sl_flat_map_insert(app->object_map[obj->type], &cstr, &obj, NULL);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
exit:
  return app_err;
error:
  goto exit;
}

static enum app_error
unregister_object(struct app* app, struct app_object* obj)
{
  const char* cstr = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  assert(obj && obj->name && app && obj->type != APP_NB_OBJECT_TYPES);

  SL(string_get(obj->name, &cstr));

  sl_err = sl_flat_map_erase(app->object_map[obj->type], &cstr, NULL);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
exit:
  return app_err;
error:
  goto exit;
}

static bool
is_object_registered(struct app* app, struct app_object* obj)
{
  void* data = NULL;
  const char* cstr = NULL;
  assert(obj && obj->name && app && obj->type != APP_NB_OBJECT_TYPES);

  SL(string_get(obj->name, &cstr));
  SL(flat_map_find(app->object_map[obj->type], &cstr, &data));
  return (data != NULL);
}

/*******************************************************************************
 *
 * Object functions.
 *
 ******************************************************************************/
enum app_error
app_init_object
  (struct app* app,
   struct app_object* obj,
   enum app_object_type type,
   const char* name)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app || !obj || type == APP_NB_OBJECT_TYPES) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  memset(obj, 0, sizeof(struct app_object));

  obj->type = type;
  sl_err = sl_create_string(NULL, app->allocator, &obj->name);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  app_err = app_set_object_name(app, obj, name ? name : "unamed");
  if(app_err != APP_NO_ERROR)
    goto error;

exit:
  return app_err;
error:
  APP(release_object(app, obj));
  goto exit;
}

enum app_error
app_release_object(struct app* app, struct app_object* obj)
{
  if(!obj || !app)
    return APP_INVALID_ARGUMENT;

  if(obj->name) {
    if(is_object_registered(app, obj)) {
      enum app_error app_err UNUSED = unregister_object(app, obj);
      assert(app_err == APP_NO_ERROR);
    }
    SL(free_string(obj->name));
  }
  return APP_NO_ERROR;
}

extern enum app_error
app_set_object_name(struct app* app, struct app_object* obj, const char* name)
{
  char scratch[256] = { [0] = '\0' };
  const char* cstr = NULL;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool empty_name = false;

  if(!app || !obj || !name) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }

  #define CALL(func) \
    do { \
      if(SL_NO_ERROR != (sl_err = func)) { \
        app_err = sl_to_app_error(sl_err); \
        goto error; \
      } \
    } while(0)

  SL(is_string_empty(obj->name, &empty_name));
  if(!empty_name)
    SL(string_get(obj->name, &cstr));

  if(empty_name || strncmp(cstr, name, strlen(name)) != 0) {
    bool erase = false;
    size_t len = 0;
    size_t i = 0;

    if(empty_name == false) {
      /* Save the previous object name into the scratch. */
      if(strlen(cstr) + 1 > sizeof(scratch)/sizeof(char)) {
        app_err = APP_MEMORY_ERROR;
        goto error;
      }
      strcpy(scratch, cstr);
      /* Unregister the object in order to keep the map order.*/
      app_err = unregister_object(app, obj);
      assert(app_err == APP_NO_ERROR);
    }

    CALL(sl_string_set(obj->name, name));
    SL(string_length(obj->name, &len));

    erase = false;
    for(i = 0; is_object_registered(app, obj) && i < SIZE_MAX; ++i) {
      char buf[32] = { [31] = '\0' };
      snprintf(buf, 15, "_%zu", i);
      if(erase) {
        SL(string_erase(obj->name, len, SIZE_MAX));
      } else {
        erase = true;
      }
      CALL(sl_string_append(obj->name, buf));
    }
    if(i >= SIZE_MAX) {
      app_err = APP_MEMORY_ERROR;
      goto error;
    }
    app_err = register_object(app, obj);
    assert(app_err == APP_NO_ERROR);
  }

  #undef CALL

exit:
  return app_err;
error:
  if(empty_name) {
    SL(clear_string(obj->name));
  } else if(scratch[0] != '\0') {
    SL(string_set(obj->name, scratch));
    app_err = register_object(app, obj);
    assert(app_err == APP_NO_ERROR);
  }
  goto exit;
}

enum app_error
app_object_name
  (struct app* app,
   const struct app_object* obj,
   const char** name)
{
  bool is_empty = false;

  if(!app || !obj || !name)
    return APP_INVALID_ARGUMENT;
  SL(is_string_empty(obj->name, &is_empty));
  if(is_empty) {
   *name = NULL;
  } else {
    SL(string_get(obj->name, name));
  }
  return APP_NO_ERROR;
}

enum app_error
app_get_object
  (struct app* app,
   enum app_object_type type,
   const char* name,
   struct app_object** out_obj)
{
  struct app_object** obj = NULL;
  if(!app || type == APP_NB_OBJECT_TYPES || !name || !out_obj)
    return APP_INVALID_ARGUMENT;
  SL(flat_map_find(app->object_map[type], &name, (void**)&obj));
  *out_obj = *obj;
  return APP_NO_ERROR;
}

enum app_error
app_object_name_completion
  (struct app* app,
   enum app_object_type type,
   const char* name,
   size_t name_len,
   size_t* completion_list_len,
   const char** completion_list[])
{
  const char** name_list = NULL;
  size_t len = 0;
  enum app_error app_err = APP_NO_ERROR;

  if(!app
  || (name_len && !name)
  || !completion_list_len
  || !completion_list
  || type == APP_NB_OBJECT_TYPES) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  SL(flat_map_key_buffer
    (app->object_map[type], &len, NULL, NULL, (void**)&name_list));
  if(0 == name_len) {
    *completion_list_len = len;
    *completion_list = name_list;
  } else {
    #define CHARBUF_SIZE 256
    char buf[CHARBUF_SIZE];
    const char* ptr = buf;
    size_t begin = 0;
    size_t end = 0;

    /* Copy the name in a mutable buffer. */
    if(name_len > CHARBUF_SIZE - 1) {
      app_err = APP_MEMORY_ERROR;
      goto error;
    }
    strncpy(buf, name, name_len);
    buf[name_len] = '\0';
    /* Find the completion range. */
    SL(flat_map_lower_bound(app->object_map[type], &ptr, &begin));
    buf[name_len] = 127;
    buf[name_len + 1] = '\0';
    SL(flat_map_upper_bound(app->object_map[type], &ptr, &end));
    /* Define the completion list. */
    *completion_list = name_list + begin;
    *completion_list_len = (name_list + end) - (*completion_list);
    if(0 == *completion_list_len)
      *completion_list = NULL;
    #undef CHARBUF_SIZE
  }
exit:
  return app_err;
error:
  goto exit;
}

