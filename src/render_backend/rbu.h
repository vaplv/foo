#ifndef RBU_H
#define RBU_H

#include "sys/ref_count.h"

#ifndef NDEBUG
  #include <assert.h>
  #define RBU(func) assert(0 == rbu_##func)
#else
  #define RBU(func) rbu_##func
#endif

/*******************************************************************************
 *
 * Forward declaration
 *
 ******************************************************************************/
struct rbi;
struct rb_buffer;
struct rb_context;
struct rb_vertex_array;

/*******************************************************************************
 *
 * Render backend utils data structure.
 *
 ******************************************************************************/
struct rbu_quad {
  struct ref ref;
  const struct rbi* rbi; /* != NULL if the quad is correctly initialized. */
  struct rb_context* ctxt;
  struct rb_buffer* vertex_buffer;
  struct rb_vertex_array* vertex_array;
};

/*******************************************************************************
 *
 * Render backend utils functions prototypes.
 *
 ******************************************************************************/
/* Create a non textured 2D quad. The vertex position attrib is bound to
 * location 0. */
extern int
rbu_init_quad
  (const struct rbi* rbi,
   struct rb_context* ctxt,
   float x,
   float y,
   float width,
   float height,
   struct rbu_quad* quad);

extern int
rbu_quad_ref_get
  (struct rbu_quad* quad);

extern int
rbu_quad_ref_put
  (struct rbu_quad* quad);

extern int
rbu_draw_quad
  (struct rbu_quad* quad);

#endif /* RBU_H */

