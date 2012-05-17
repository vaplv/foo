#ifndef APP_C_H
#define APP_C_H

#include "app/core/regular/app_object.h"
#include "app/core/app.h"
#include "renderer/rdr_attrib.h"
#include "resources/rsrc.h"
#include "resources/rsrc_geometry.h"
#include "stdlib/sl.h"
#include "stdlib/sl_logger.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include <stdbool.h>

#define APP_PRINT_MSG(logger, ...) \
  SL(logger_print((logger), __VA_ARGS__))

#define APP_ERR_PREFIX "error: "
#define APP_PRINT_ERR(logger, ...) \
  SL(logger_print((logger), APP_ERR_PREFIX __VA_ARGS__))

#define APP_WARN_PREFIX "warning: "
#define APP_PRINT_WARN(logger, ...) \
  SL(logger_print((logger), APP_WARN_PREFIX __VA_ARGS__))

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
  struct sl_flat_map* object_map[APP_NB_OBJECT_TYPES]; /* {char*, object*}. */

  struct command_system {
    char scratch[1024];
    FILE* stream;
    struct sl_hash_table* htbl; /* htbl of commands. */
    /* set of const char*. Used by completion and ls.*/
    struct sl_flat_set* name_set;
  } cmd;

  struct cvar_system {
    struct sl_flat_map* map; /* Associative container name -> cvar. */
    /* Put the builtin cvar here. */
    const struct app_cvar* project_path;
    const struct app_cvar* show_aabb;
  } cvar_system;

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

/* Factorize the completion process of data structure stored into a flat map
 * with respect to their name. */
extern enum app_error
app_mapped_name_completion
  (struct sl_flat_map* map, /* Assume that the map key type is char* */
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

