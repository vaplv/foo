#ifndef RDR_MODEL_C_H
#define RDR_MODEL_C_H

#include <stdbool.h>
#include <stddef.h>

enum rdr_model_signal {
  RDR_MODEL_SIGNAL_UPDATE_DATA,
  RDR_NB_MODEL_SIGNALS
};

struct rb_uniform;
struct rb_attrib;
struct rdr_model;
struct rdr_model_callback;
struct rdr_system;

struct rdr_model_desc {
  struct rb_attrib** attrib_list;
  struct rdr_uniform* uniform_list;
  size_t nb_attribs;
  size_t nb_uniforms;
  size_t sizeof_attrib_data;
  size_t sizeof_uniform_data;
};

extern enum rdr_error
rdr_bind_model
  (struct rdr_system* sys,
   struct rdr_model* model,
   size_t* out_nb_indices,
   int flag); /* Combination of enum rdr_bind_flag */

extern enum rdr_error
rdr_get_model_desc
  (struct rdr_model* model,
   struct rdr_model_desc* desc);

extern enum rdr_error
rdr_attach_model_callback
  (struct rdr_model* model,
   enum rdr_model_signal signal,
   void (*func)(struct rdr_model*, void*),
   void* data);

extern enum rdr_error
rdr_detach_model_callback
  (struct rdr_model* model,
   enum rdr_model_signal signal,
   void (*func)(struct rdr_model*,void*),
   void* data);

extern enum rdr_error
rdr_is_model_callback_attached
  (struct rdr_model* model,
   enum rdr_model_signal signal,
   void (*func)(struct rdr_model*,void*),
   void* data,
   bool* is_attached);

#endif /* RDR_MODEL_C_H */

