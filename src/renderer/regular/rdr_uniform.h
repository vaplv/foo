#ifndef RDR_UNIFORM_H
#define RDR_UNIFORM_H

struct rb_uniform;

enum rdr_uniform_usage {
  RDR_PROJECTION_UNIFORM,
  RDR_MODELVIEW_UNIFORM,
  RDR_MODELVIEWPROJ_UNIFORM,
  RDR_MODELVIEW_INVTRANS_UNIFORM,
  RDR_DRAW_ID_UNIFORM,
  RDR_REGULAR_UNIFORM
};

struct rdr_uniform {
  struct rb_uniform* uniform;
  enum rdr_uniform_usage usage;
};

#endif /* RDR_UNIFORM_H */

