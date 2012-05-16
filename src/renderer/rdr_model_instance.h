#ifndef RDR_MODEL_INSTANCE_H
#define RDR_MODEL_INSTANCE_H

#include "renderer/rdr_attrib.h"
#include "renderer/rdr_error.h"
#include <stdbool.h>

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

struct aosf44;
struct rdr_model;
struct rdr_model_instance;
struct rdr_system;

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
rdr_model_instance_ref_get
  (struct rdr_model_instance* instance);

extern enum rdr_error
rdr_model_instance_ref_put
  (struct rdr_model_instance* instance);

extern enum rdr_error
rdr_get_model_instance_uniforms
  (struct rdr_model_instance* instance,
   size_t* nb_uniforms,
   struct rdr_model_instance_data* uniform_list);

extern enum rdr_error
rdr_get_model_instance_attribs
  (struct rdr_model_instance* instance,
   size_t* nb_attribs,
   struct rdr_model_instance_data* attrib_list);

extern enum rdr_error
rdr_model_instance_transform
  (struct rdr_model_instance* instance,
   const float transform[16]);

extern enum rdr_error
rdr_get_model_instance_transform
  (const struct rdr_model_instance* instance,
   float transform[16]);

extern enum rdr_error
rdr_translate_model_instances
  (struct rdr_model_instance* instance_list[],
   size_t nb_instances,
   bool local_translation,
   const float translation[3]);

extern enum rdr_error
rdr_rotate_model_instances
  (struct rdr_model_instance* instance_list[],
   size_t nb_instances,
   bool local_rotation,
   const float rotation[3]);

extern enum rdr_error
rdr_scale_model_instances
  (struct rdr_model_instance* instance_list[],
   size_t nb_instances,
   bool local_scale,
   const float scale[3]);

extern enum rdr_error
rdr_move_model_instances
  (struct rdr_model_instance* instance_list[],
   size_t nb_instances,
   const float pos[3]);

extern enum rdr_error
rdr_transform_model_instances
  (struct rdr_model_instance* instance_list[],
   size_t nb_instances,
   bool local_transformation,
   const struct aosf44* transform);

extern enum rdr_error
rdr_model_instance_material_density
  (struct rdr_model_instance* instance,
   enum rdr_material_density density);

extern enum rdr_error
rdr_get_model_instance_material_density
  (const struct rdr_model_instance* instance,
   enum rdr_material_density* out_density);

extern enum rdr_error
rdr_model_instance_rasterizer
  (struct rdr_model_instance* instance,
   const struct rdr_rasterizer_desc* raster_desc);

extern enum rdr_error
rdr_get_model_instance_rasterizer
  (const struct rdr_model_instance* instance,
   struct rdr_rasterizer_desc* out_raster_desc);

extern enum rdr_error
rdr_get_model_instance_model
  (struct rdr_model_instance* instance,
   struct rdr_model** mdl);

/* Get the bounds of axis aligned bounding box of the instance, i.e. the aabb
 * of the mesh model transformed by the instance transform. */
extern enum rdr_error
rdr_get_model_instance_aabb
  (const struct rdr_model_instance* instance,
   float min_bound[3],
   float max_bound[3]);

extern enum rdr_error
rdr_attach_model_instance_callback
  (struct rdr_model_instance* instance,
   void (*func)(struct rdr_model_instance*, void*), 
   void* data);

extern enum rdr_error
rdr_detach_model_instance_callback
  (struct rdr_model_instance* instance,
   void (*func)(struct rdr_model_instance*, void*),
   void* data);

extern enum rdr_error
rdr_is_model_instance_callback_attached
  (struct rdr_model_instance* instance,
   void (*func)(struct rdr_model_instance*, void*),
   void* data,
   bool* is_attached);

#endif /* RDR_MODEL_INSTANCE_H */

