#ifndef APP_C_H
#define APP_C_H

#include "app/core/app.h"
#include "renderer/rdr_attrib.h"
#include "resources/rsrc.h"
#include "resources/rsrc_geometry.h"
#include "stdlib/sl.h"
#include "stdlib/sl_logger.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include <stdbool.h>

#define APP_LOG_MSG(logger, ...) \
  SL(logger_print((logger), __VA_ARGS__))

#define APP_ERR_PREFIX "error: "
#define APP_LOG_ERR(logger, ...) \
  SL(logger_print((logger), APP_ERR_PREFIX __VA_ARGS__))

#define APP_WARN_PREFIX "warning: "
#define APP_LOG_WARN(logger, ...) \
  SL(logger_print((logger), APP_WARN_PREFIX __VA_ARGS__))

enum app_object_type {
  APP_MODEL,
  APP_MODEL_INSTANCE,
  APP_NB_OBJECT_TYPES
};

struct app_model;
struct app_model_instance;
struct app_view;
struct app_world;
struct rdr_frame;
struct rdr_material;
struct rdr_system;
struct rdr_term;
struct rdr_world;
struct rsrc_context;
struct rsrc_font;
struct rsrc_wavefront_obj;
struct sl_flat_set;
struct sl_logger;
struct sl_vector;
struct wm_device;
struct wm_window;

struct app {
  struct ref ref;
  struct app_world* world; /* Main world. */
  struct app_view* view; /* Main view. */
  struct mem_allocator* allocator; /* allocator of this (app). */
  struct sl_logger* logger;
  struct sl_flat_set* callback_list[APP_NB_SIGNALS];
  struct sl_flat_map* object_map[APP_NB_OBJECT_TYPES]; /* maps of {char*, object*}. */

  struct command_system {
    ALIGN(16) char scratch[1024];
    FILE* stream;
    struct sl_hash_table* htbl; /* htbl of commands. */
    struct sl_flat_set* name_set; /* set of const char*. Used by completion and ls.*/
  } cmd;

  struct term {
    struct rdr_term* render_term;
    struct app_command_buffer* cmdbuf;
    bool is_enabled;
  } term;

  struct renderer {
    struct rdr_font* term_font;
    struct rdr_frame* frame;
    struct rdr_material* default_material;
    struct rdr_system* system;
    struct mem_allocator allocator;
  } rdr;

  struct window_manager {
    struct wm_device* device;
    struct wm_window* window;
    struct mem_allocator allocator;
  } wm;

  struct resources {
    struct rsrc_context* context;
    struct rsrc_wavefront_obj* wavefront_obj;
    struct rsrc_font* font;
    struct mem_allocator allocator;
  } rsrc;

  bool post_exit;
};

extern enum app_error
app_register_object
  (struct app* app,
   enum app_object_type type,
   const char* key,
   void* object);

extern enum app_error
app_unregister_object
  (struct app* app,
   enum app_object_type type,
   const char* name);

extern enum app_error
app_is_object_registered
  (struct app* app,
   enum app_object_type type,
   const char* name,
   bool* is_registered);

extern enum app_error
app_get_registered_object
  (struct app* app,
   enum app_object_type type,
   const char* name,
   void** object);

extern enum app_error
app_object_name_completion
  (struct app* app,
   enum app_object_type type,
   const char* name,
   size_t name_len,
   size_t* completion_list_len,
   const char** completion_list[]);

extern enum app_error
app_invoke_callbacks
  (struct app* app,
   enum app_signal signal,
   ...);

extern enum rdr_attrib_usage
rsrc_to_rdr_attrib_usage
  (enum rsrc_attrib_usage usage);

extern enum rdr_type
rsrc_to_rdr_type
  (enum rsrc_type type);

#endif /* APP_C_H */

