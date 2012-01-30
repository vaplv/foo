#ifndef RSRC_WAVEFRONT_OBJ_C_H
#define RSRC_WAVEFRONT_OBJ_C_H

#include <stdbool.h>
#include <stddef.h>

struct rsrc_wavefront_obj_line {
  size_t v;
  size_t vt;
};

struct rsrc_wavefront_obj_face {
  size_t v;
  size_t vt;
  size_t vn;
};

struct rsrc_wavefront_obj_range {
  size_t begin;
  size_t end;
};

struct rsrc_wavefront_obj_group {
  struct sl_vector* name_list; /* vector of char*. */
  struct rsrc_wavefront_obj_range point_range;
  struct rsrc_wavefront_obj_range line_range;
  struct rsrc_wavefront_obj_range face_range;
};

struct rsrc_wavefront_obj_smooth_group {
  struct rsrc_wavefront_obj_range face_range;
  bool is_on;
};

struct rsrc_wavefront_obj_mtl {
  char* name;
  struct rsrc_wavefront_obj_range point_range;
  struct rsrc_wavefront_obj_range line_range;
  struct rsrc_wavefront_obj_range face_range;
};

struct rsrc_wavefront_obj {
  struct rsrc_context* ctxt;
  struct sl_vector* position_list; /* vector of float[3]. */
  struct sl_vector* normal_list; /* vector of float[3]. */
  struct sl_vector* texcoord_list; /* vector of float[3]. */
  struct sl_vector* point_list; /* vector of vector of size_t. */
  struct sl_vector* line_list; /* vector of vector of line. */
  struct sl_vector* face_list; /* vector of vector of face. */
  struct sl_vector* group_list; /* vector of group. */
  struct sl_vector* smooth_group_list; /* vector of smooth_group. */
  struct sl_vector* mtllib_list; /* vector of char*. */
  struct sl_vector* mtl_list; /* vector of mtl. */
};

#endif /* RSRC_WAVEFRONT_OBJ_C_H */

