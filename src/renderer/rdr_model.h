#ifndef RDR_MODEL_H
#define RDR_MODEL_H

#include "renderer/rdr_error.h"

struct rdr_material;
struct rdr_mesh;
struct rdr_system;

struct rdr_model;

extern enum rdr_error
rdr_create_model
  (struct rdr_system* sys,
   struct rdr_mesh* mesh,
   struct rdr_material* mtr,
   struct rdr_model** out_mdl);

extern enum rdr_error
rdr_model_ref_get
  (struct rdr_model* mdl);

extern enum rdr_error
rdr_model_ref_put
  (struct rdr_model* mdl);

extern enum rdr_error
rdr_model_mesh
  (struct rdr_model* mdl,
   struct rdr_mesh* mesh);

extern enum rdr_error
rdr_model_material
  (struct rdr_model* mdl,
   struct rdr_material* mtr);

#endif /* RDR_MODEL_H */

