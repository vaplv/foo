#ifndef RDR_DRAW_DESC_H
#define RDR_DRAW_DESC_H

#include "sys/sys.h"

struct rb_program;
struct rdr_uniform;

enum rdr_bind_flag {
  RDR_BIND_ATTRIB_POSITION = BIT(0),
  RDR_BIND_NONE = 0,
  RDR_BIND_ALL = ~0
};

struct rdr_draw_desc {
  struct rdr_uniform* uniform_list;
  size_t nb_uniforms;
  int bind_flag; /* combination of rdr bind flags. */
};

#endif /* RDR_DRAW_DESC_H */

