#ifndef APP_C_H
#define APP_C_H

#include "renderer/rdr_attrib.h"
#include "renderer/rdr_error.h"
#include "resources/rsrc.h"
#include "resources/rsrc_error.h"
#include "resources/rsrc_geometry.h"
#include "stdlib/sl_error.h"

#ifndef NDEBUG
  #include <assert.h>
  #define SL(func) assert(SL_NO_ERROR == sl_##func)
  #define RSRC(func) assert(RSRC_NO_ERROR == rsrc_##func)
  #define RDR(func) assert(RDR_NO_ERROR == rdr_##func)
  #define WM(func) assert(WM_NO_ERROR == wm_##func)
#else
  #define SL(func) sl_##func
  #define RSRC(func) rsrc_##func
  #define RDR(func) rdr_##func
  #define WM(func) wm_##func
#endif

struct app_view;
struct app_world;
struct rdr_material;
struct rdr_system;
struct rdr_world;
struct rsrc_context;
struct rsrc_wavefront_obj;
struct sl_context;
struct sl_vector;
struct wm_device;
struct wm_window;

struct app {
  struct app_world* world;
  struct app_view* view;
  struct rdr_material* default_render_material;
  struct rdr_system* rdr;
  struct rsrc_context* rsrc;
  struct rsrc_wavefront_obj* wavefront_obj;
  struct sl_context* sl;
  struct sl_vector* model_list; /* vector of app_model*. */
  struct sl_vector* model_instance_list; /* vector of app_model_instance* .*/
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

