#ifndef RDR_MODEL_C_H
#define RDR_MODEL_C_H

#include "renderer/regular/rdr_object.h"
#include <stddef.h>

enum rdr_uniform_usage {
  RDR_PROJECTION_UNIFORM,
  RDR_MODELVIEW_UNIFORM,
  RDR_VIEWPROJ_UNIFORM,
  RDR_MODELVIEW_INVTRANS_UNIFORM,
  RDR_REGULAR_UNIFORM
};

struct rb_uniform;
struct rb_attrib;
struct rdr_model;
struct rdr_model_callback;

struct rdr_model_callback_desc {
  void* data;
  enum rdr_error (*func)(struct rdr_system*, struct rdr_model*, void* data);
};

struct rdr_model_desc {
  struct rb_attrib** attrib_list;
  struct rdr_model_uniform {
    struct rb_uniform* uniform;
    enum rdr_uniform_usage usage;
  }* uniform_list;
  size_t nb_attribs;
  size_t nb_uniforms;
  size_t sizeof_attrib_data;
  size_t sizeof_uniform_data;
};

RDR_OBJECT(struct rdr_model);

extern enum rdr_error
rdr_bind_model
  (struct rdr_system* sys,
   struct rdr_model* model,
   size_t* out_nb_indices);

extern enum rdr_error
rdr_get_model_desc
  (struct rdr_system* sys,
   struct rdr_model* model,
   struct rdr_model_desc* desc);

extern enum rdr_error
rdr_attach_model_callback
  (struct rdr_system* sys,
   struct rdr_model* model,
   const struct rdr_model_callback_desc* callback_desc,
   struct rdr_model_callback** out_callback);

extern enum rdr_error
rdr_detach_model_callback
  (struct rdr_system* sys,
   struct rdr_model* model,
   struct rdr_model_callback* callback);

#endif /* RDR_MODEL_C_H */

