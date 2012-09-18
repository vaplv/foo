#ifndef RDR_MATERIAL_H
#define RDR_MATERIAL_H

#include "renderer/rdr.h"
#include "renderer/rdr_error.h"

enum rdr_shader_usage {
  RDR_VERTEX_SHADER,
  RDR_GEOMETRY_SHADER,
  RDR_FRAGMENT_SHADER,
  /* */
  RDR_NB_SHADER_USAGES
};

struct rdr_system;
struct rdr_material;

RDR_API enum rdr_error
rdr_create_material
  (struct rdr_system* sys,
   struct rdr_material** out_mtr);

RDR_API enum rdr_error
rdr_material_ref_get
  (struct rdr_material* mtr);

RDR_API enum rdr_error
rdr_material_ref_put
  (struct rdr_material* mtr);

RDR_API enum rdr_error
rdr_get_material_log
  (struct rdr_material* mtr,
   const char** out_log);

RDR_API enum rdr_error
rdr_material_program
  (struct rdr_material* mtr,
   const char* shader_sources[RDR_NB_SHADER_USAGES]);

#endif /* RDR_MATERIAL_H */

