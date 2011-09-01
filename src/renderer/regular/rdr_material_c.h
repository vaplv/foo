#ifndef RDR_MATERIAL_C_H
#define RDR_MATERIAL_C_H

#include "renderer/regular/rdr_object.h"
#include "renderer/rdr_attrib.h"
#include "renderer/rdr_error.h"
#include <stdbool.h>

struct rb_attrib;
struct rb_uniform;
struct rdr_system;
struct rdr_material;
struct rdr_material_callback;

struct rdr_material_callback_desc {
  void* data;
  enum rdr_error (*func)(struct rdr_system*, struct rdr_material*, void* data);
};

struct rdr_material_desc {
  struct rb_attrib** attrib_list;
  struct rb_uniform** uniform_list;
  size_t nb_attribs;
  size_t nb_uniforms;
};

RDR_OBJECT(struct rdr_material);

extern enum rdr_error
rdr_bind_material
  (struct rdr_system* sys,
   struct rdr_material* mtr);

extern enum rdr_error
rdr_get_material_desc
  (struct rdr_system* sys,
   struct rdr_material* mtr,
   struct rdr_material_desc* desc);

extern enum rdr_error
rdr_is_material_linked
  (struct rdr_system* sys,
   struct rdr_material* mtr,
   bool* is_linked);

extern enum rdr_error
rdr_attach_material_callback
  (struct rdr_system* sys,
   struct rdr_material* mtr,
   const struct rdr_material_callback_desc* cbk,
   struct rdr_material_callback** out_cbk);

extern enum rdr_error
rdr_detach_material_callback
  (struct rdr_system* sys,
   struct rdr_material* mtr,
   struct rdr_material_callback* cbk);

#endif /* RDR_MATERIAL_C_H */

