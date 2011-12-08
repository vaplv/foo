#ifndef APP_C_H
#define APP_C_H

#include "app/core/app_error.h"
#include "renderer/rdr_attrib.h"
#include "renderer/rdr_error.h"
#include "resources/rsrc.h"
#include "resources/rsrc_error.h"
#include "resources/rsrc_geometry.h"
#include "stdlib/sl_error.h"
#include "stdlib/sl_logger.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"
#include <stdbool.h>

#ifndef NDEBUG
  #include <assert.h>
  #define APP(func) assert(APP_NO_ERROR == app_##func)
  #define SL(func) assert(SL_NO_ERROR == sl_##func)
  #define RSRC(func) assert(RSRC_NO_ERROR == rsrc_##func)
  #define RDR(func) assert(RDR_NO_ERROR == rdr_##func)
  #define SYS(func) assert(SYS_NO_ERROR == sys_##func)
  #define WM(func) assert(WM_NO_ERROR == wm_##func)
#else
  #define APP(func) app_##func
  #define SL(func) sl_##func
  #define RSRC(func) rsrc_##func
  #define RDR(func) rdr_##func
  #define SYS(func) sys_##func
  #define WM(func) wm_##func
#endif

#define APP_LOG_MSG(app, ...) \
  SL(logger_print((app)->logger, __VA_ARGS__))

#define APP_ERR_PREFIX "error: "
#define APP_LOG_ERR(app, ...) \
  SL(logger_print((app)->logger, APP_ERR_PREFIX __VA_ARGS__))

struct app_model;
struct app_model_instance;
struct app_view;
struct app_world;
struct rdr_material;
struct rdr_system;
struct rdr_world;
struct rsrc_context;
struct rsrc_wavefront_obj;
struct sl_logger;
struct sl_vector;
struct wm_device;
struct wm_window;

struct model_callback {
  void (*func)(struct app_model*, void*);
  void* data;
};

struct app {
  struct ref ref;
  struct app_world* world;
  struct app_view* view;
  struct mem_allocator* allocator; /* allocator of this (app). */
  struct mem_allocator rdr_allocator; /* allocator used by the renderer. */
  struct mem_allocator rsrc_allocator; /* allocator used by the resources. */
  struct mem_allocator wm_allocator; /* alloctor used by the window manger. */
  struct rdr_material* default_render_material;
  struct rdr_system* rdr;
  struct rsrc_context* rsrc;
  struct rsrc_wavefront_obj* wavefront_obj;
  struct sl_logger* logger;
  struct sl_sorted_vector* add_model_cbk_list; /* vector of callbacks. */
  struct sl_sorted_vector* remove_model_cbk_list; /* vector of callback. */
  struct sl_sorted_vector* model_list; /* vector of app_model*. */
  struct sl_sorted_vector* model_instance_list; /* vec of app_model_instance*.*/
  struct wm_device* wm;
  struct wm_window* window;
};

extern enum rdr_attrib_usage
rsrc_to_rdr_attrib_usage
  (enum rsrc_attrib_usage usage);

extern enum rdr_type
rsrc_to_rdr_type
  (enum rsrc_type type);

#endif /* APP_C_H */

