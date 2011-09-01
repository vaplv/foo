#ifndef RDR_MODEL_INSTANCE_H
#define RDR_MODEL_INSTANCE_H

#include "renderer/rdr_attrib.h"
#include "renderer/rdr_error.h"

enum rdr_material_density {
  RDR_OPAQUE,
  RDR_TRANSLUCENT
};

enum rdr_fill_mode {
  RDR_WIREFRAME,
  RDR_SOLID
};

enum rdr_cull_mode {
  RDR_CULL_FRONT,
  RDR_CULL_BACK,
  RDR_CULL_NONE
};

struct rdr_model;
struct rdr_model_instance;
struct rdr_model_instance_callback;
struct rdr_system;

struct rdr_model_instance_callback_desc {
  void* data;
  enum rdr_error (*func)(struct rdr_system*, struct rdr_model_instance*, void*);
};

struct rdr_model_instance_data {
  enum rdr_type type;
  const char* name;
  void* data;
};

struct rdr_rasterizer_desc {
  enum rdr_cull_mode cull_mode;
  enum rdr_fill_mode fill_mode;
};

extern enum rdr_error
rdr_create_model_instance
  (struct rdr_system* sys,
   struct rdr_model* model,
   struct rdr_model_instance** out_instance);

extern enum rdr_error
rdr_free_model_instance
  (struct rdr_system* sys,
   struct rdr_model_instance* instance);

extern enum rdr_error
rdr_get_model_instance_uniforms
  (struct rdr_system* sys,
   struct rdr_model_instance* instance,
   size_t* nb_uniforms,
   struct rdr_model_instance_data* uniform_list);

extern enum rdr_error
rdr_get_model_instance_attribs
  (struct rdr_system* sys,
   struct rdr_model_instance* instance,
   size_t* nb_attribs,
   struct rdr_model_instance_data* attrib_list);

extern enum rdr_error
rdr_model_instance_transform
  (struct rdr_system* sys,
   struct rdr_model_instance* instance,
   const float transform[16]);

extern enum rdr_error
rdr_get_model_instance_transform
  (struct rdr_system* sys,
   const struct rdr_model_instance* instance,
   float transform[16]);

extern enum rdr_error
rdr_model_instance_material_density
  (struct rdr_system* sys,
   struct rdr_model_instance* instance,
   enum rdr_material_density density);

extern enum rdr_error
rdr_get_model_instance_material_density
  (struct rdr_system* sys,
   const struct rdr_model_instance* instance,
   enum rdr_material_density* out_density);

extern enum rdr_error
rdr_model_instance_rasterizer
  (struct rdr_system* sys,
   struct rdr_model_instance* instance,
   const struct rdr_rasterizer_desc* raster_desc);

extern enum rdr_error
rdr_get_model_instance_rasterizer
  (struct rdr_system* sys,
   const struct rdr_model_instance* instance,
   struct rdr_rasterizer_desc* out_raster_desc);

extern enum rdr_error
rdr_attach_model_instance_callback
  (struct rdr_system* sys,
   struct rdr_model_instance* instance,
   const struct rdr_model_instance_callback_desc* cbk_desc,
   struct rdr_model_instance_callback** out_cbk);

extern enum rdr_error
rdr_detach_model_instance_callback
  (struct rdr_system* sys,
   struct rdr_model_instance* instance,
   struct rdr_model_instance_callback* cbk);

#endif /* RDR_MODEL_INSTANCE_H */

