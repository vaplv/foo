#ifndef RDR_WORLD_H
#define RDR_WORLD_H

#include "renderer/rdr_error.h"
#include "sys/sys.h"

struct rdr_view {
  ALIGN(16) float transform[16];
  float proj_ratio;
  float fov_x;
  float znear, zfar;
  unsigned int x, y, width, height;
};

struct rdr_model_instance;
struct rdr_system;
struct rdr_world;

extern enum rdr_error
rdr_create_world
  (struct rdr_system* sys,
   struct rdr_world** out_world);

extern enum rdr_error
rdr_world_ref_get
  (struct rdr_world* world);

extern enum rdr_error
rdr_world_ref_put
  (struct rdr_world* world);

extern enum rdr_error
rdr_add_model_instance
  (struct rdr_world* world,
   struct rdr_model_instance* instance);

extern enum rdr_error
rdr_remove_model_instance
  (struct rdr_world* world,
   struct rdr_model_instance* instance);

extern enum rdr_error
rdr_draw_world
  (struct rdr_world* world,
   const struct rdr_view* view);

extern enum rdr_error
rdr_background_color
  (struct rdr_world* world,
   const float rgb[3]);

#endif /* RDR_WORLD_H */

