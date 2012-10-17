#ifndef RDR_SYSTEM_C_H
#define RDR_SYSTEM_C_H

#include "render_backend/rbi.h"
#include "render_backend/rbu.h"
#include "renderer/regular/rdr_imdraw_c.h"
#include "sys/mem_allocator.h"
#include "sys/ref_count.h"

#define RDR_ERRBUF_LEN 1024

struct rb_context;
struct sl_logger;

struct rdr_system {
  struct mem_allocator* allocator;
  struct mem_allocator render_backend_allocator;
  struct ref ref;
  struct sl_logger* logger;

  /* Render backend data. */
  struct rbi rb; 
  struct rb_context* ctxt;
  struct rb_config cfg;

  /* im rendering. */
  struct im_rendering {
    struct im_draw {
      struct rb_shader* vertex_shader;
      struct rb_shader* fragment_shader;
      struct rb_program* shading_program;
      struct rb_uniform* transform;
      struct rb_uniform* color;
    } draw2d, draw3d, draw2d_color, draw2d_picking, draw3d_picking;
    struct im_grid {
      struct rdr_im_grid_desc desc;
      struct rb_buffer* vertex_buffer;
      struct rb_vertex_array* vertex_array;
      size_t sizeof_vertex_buffer;
      unsigned int nvertices;
    } grid;
    struct im_line {
      struct rb_buffer* vertex_buffer;
      struct rb_vertex_array* vertex_array;
    } line;
  } im;

  /* Render backend utils. */
  struct render_backend_utils {
    struct rbu_geometry circle;
    struct rbu_geometry cone;
    struct rbu_geometry cylinder;
    struct rbu_geometry quad;
    struct rbu_geometry solid_parallelepiped;
    struct rbu_geometry wire_parallelepiped;
  } rbu;
};

#endif /* RDR_SYSTEM_C_H. */

