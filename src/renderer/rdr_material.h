#ifndef RDR_MATERIAL_H
#define RDR_MATERIAL_H

#include "renderer/rdr_error.h"

enum rdr_shader_usage {
  RDR_VERTEX_SHADER,
  RDR_GEOMETRY_SHADER,
  RDR_FRAGMENT_SHADER,
};

enum { 
  RDR_NB_SHADER_USAGES = 3
};

struct rdr_system;
struct rdr_material;

extern enum rdr_error
rdr_create_material
  (struct rdr_system* sys,
   struct rdr_material** out_mtr);

extern enum rdr_error
rdr_free_material
  (struct rdr_system* sys,
   struct rdr_material* mtr);

extern enum rdr_error
rdr_get_material_log
  (struct rdr_system* sys,
   struct rdr_material* mtr,
   const char** out_log);

extern enum rdr_error
rdr_material_program
  (struct rdr_system* sys,
   struct rdr_material* mtr,
   const char* shader_sources[RDR_NB_SHADER_USAGES]);

#endif /* RDR_MATERIAL_H */

