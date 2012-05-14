#ifndef RBU_H
#define RBU_H

#include "sys/ref_count.h"
#include <stdbool.h>
#include <stddef.h>

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
struct rbu_geometry {
  struct ref ref;
  const struct rbi* rbi; /* != NULL if the geometry is correctly initialized. */
  struct rb_context* ctxt;
  struct rb_buffer* vertex_buffer;
  struct rb_buffer* index_buffer;
  struct rb_vertex_array* vertex_array;
  size_t nb_vertices;
  enum rb_primitive_type primitive_type;
};

/*******************************************************************************
 *
 * Render backend utils functions prototypes.
 *
 ******************************************************************************/
/* Create a non textured 2D quad. The vertex position is bound to location 0. */
extern int
rbu_init_quad
  (const struct rbi* rbi,
   struct rb_context* ctxt,
   float x,
   float y,
   float width,
   float height,
   struct rbu_geometry* quad);

/* Create a 2D circle. The vertex position is bound to location 0. */
extern int
rbu_init_circle
  (const struct rbi* rbi,
   struct rb_context* ctxt,
   unsigned int npoints,
   float x,
   float y,
   float radius,
   struct rbu_geometry* circle);

extern int
rbu_init_parallelepiped
  (const struct rbi* rbi,
   struct rb_context* ctxt,
   float pos[3],
   float size[3],
   bool wireframe,
   struct rbu_geometry* cube);
   
extern int
rbu_geometry_ref_get
  (struct rbu_geometry* geom);

extern int
rbu_geometry_ref_put
  (struct rbu_geometry* geom);

extern int
rbu_draw_geometry
  (struct rbu_geometry* geom);

#endif /* RBU_H */

