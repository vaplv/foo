#ifndef APP_OBJECT_H
#define APP_OBJECT_H

#include "app/core/app_error.h"
#include "sys/ref_count.h"
#include <stdbool.h>
#include <stddef.h>

enum app_object_type {
  APP_MODEL,
  APP_MODEL_INSTANCE,
  APP_NB_OBJECT_TYPES
};

struct app;
struct sl_string;

struct app_object {
  struct sl_string* name;
  enum app_object_type type;
};

/* The initialization automatically register the object against the
 * application. */
extern enum app_error
app_init_object
  (struct app* app,
   struct app_object* obj,
   enum app_object_type type,
   const char* name); /* May be NULL. */

/* If the object is still registered the release function unregister it. */
extern enum app_error
app_release_object
  (struct app* app,
   struct app_object* obj);

/* Force the unregistration of the object. Useful to defer the release of the
 * object (e.g.: one want to delete the object but it is still referenced
 * elsewhere. */
extern enum app_error
app_unregister_object
  (struct app* app,
   struct app_object* obj);

extern enum app_error
app_is_object_registered
  (struct app* app,
   struct app_object* obj,
   bool* is_registered);

extern enum app_error
app_set_object_name
  (struct app* app,
   struct app_object* obj,
   const char* name);

extern enum app_error
app_object_name
  (struct app* app,
   const struct app_object* obj,
   const char** name);

extern enum app_error
app_get_object
  (struct app* app,
   enum app_object_type type,
   const char* name,
   struct app_object** obj); /* Set to NULL if not found. */

extern enum app_error
app_object_name_completion
  (struct app* app,
   enum app_object_type type,
   const char* name,
   size_t name_len,
   size_t* completion_list_len,
   const char** completion_list[]);

extern enum app_error
app_init_object_system
  (struct app* app);

extern enum app_error
app_shutdown_object_system
  (struct app* app);

extern enum app_error
app_clear_object_system
  (struct app* app);

#endif /* APP_OBJECT_H */

