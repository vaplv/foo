#ifndef RDR_WORLD_H
#define RDR_WORLD_H

#include "renderer/rdr.h"
#include "renderer/rdr_error.h"
#include "sys/sys.h"

ALIGN(16) struct rdr_view {
  float transform[16];
  float proj_ratio;
  float fov_x;
  float znear, zfar;
  unsigned int x, y, width, height;
};

struct rdr_model_instance;
struct rdr_system;
struct rdr_world;

RDR_API enum rdr_error
rdr_create_world
  (struct rdr_system* sys,
   struct rdr_world** out_world);

RDR_API enum rdr_error
rdr_world_ref_get
  (struct rdr_world* world);

RDR_API enum rdr_error
rdr_world_ref_put
  (struct rdr_world* world);

RDR_API enum rdr_error
rdr_add_model_instance
  (struct rdr_world* world,
   struct rdr_model_instance* instance);

RDR_API enum rdr_error
rdr_remove_model_instance
  (struct rdr_world* world,
   struct rdr_model_instance* instance);

#endif /* RDR_WORLD_H */

