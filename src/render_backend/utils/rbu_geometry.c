#include "render_backend/rbi.h"
#include "render_backend/rbu.h"
#include "sys/math.h"
#include "sys/sys.h"
#include <assert.h>
#include <math.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_geometry(struct ref* ref)
{
  struct rbu_geometry* geom = NULL;
  assert(ref);

  geom = CONTAINER_OF(ref, struct rbu_geometry, ref);
  if(LIKELY(geom->rbi != NULL)) {
    if(LIKELY(geom->vertex_buffer != NULL))
      RBI(geom->rbi, buffer_ref_put(geom->vertex_buffer));
    if(LIKELY(geom->vertex_array != NULL))
      RBI(geom->rbi, vertex_array_ref_put(geom->vertex_array));

    if(LIKELY(geom->ctxt != NULL))
      RBI(geom->rbi, context_ref_put(geom->ctxt));
    memset(geom, 0, sizeof(struct rbu_geometry));
  }
}

static FINLINE void
init_geometry
  (const struct rbi* rbi,
   struct rb_context* ctxt,
   struct rbu_geometry* geom)
{
  assert(rbi && geom);

  memset(geom, 0, sizeof(struct rbu_geometry));
  ref_init(&geom->ref);
  RBI(rbi, context_ref_get(ctxt));
  geom->ctxt = ctxt;
  geom->rbi = rbi;
}

/*******************************************************************************
 *
 * Geometry building functions.
 *
 ******************************************************************************/
EXPORT_SYM int
rbu_init_quad
  (const struct rbi* rbi,
   struct rb_context* ctxt,
   float x, float y, float w, float h,
   struct rbu_geometry* quad)
{
  const float vertices[8] = {
    x, y + h, /* First vertex. */
    x, y, /* Second vertex. */
    x + w, y + h, /* Third vertex. */
    x + w, y, /* Fourth vertex. */
  };
  struct rb_buffer_desc buf_desc;
  struct rb_buffer_attrib buf_attrib;
  int err = 0;
  memset(&buf_desc, 0, sizeof(buf_desc));

  if(UNLIKELY(quad == NULL || rbi == NULL)) {
    err = -1;
    goto error;
  }
  init_geometry(rbi, ctxt, quad);

  #define CALL(func) \
    do { \
      if(0 != (err = func)) \
        goto error; \
    } while(0)
  /* Vertex buffer. */
  buf_desc.size = sizeof(vertices);
  buf_desc.target = RB_BIND_VERTEX_BUFFER;
  buf_desc.usage = RB_USAGE_IMMUTABLE;
  CALL(rbi->create_buffer(ctxt, &buf_desc, vertices, &quad->vertex_buffer));
  /* Vertex array. */
  buf_attrib.index = 0;
  buf_attrib.stride = 2 * sizeof(float);
  buf_attrib.offset = 0;
  buf_attrib.type = RB_FLOAT2;
  CALL(rbi->create_vertex_array(ctxt, &quad->vertex_array));
  CALL(rbi->vertex_attrib_array
    (quad->vertex_array, quad->vertex_buffer, 1, &buf_attrib));
  #undef CALL

  quad->primitive_type = RB_TRIANGLE_STRIP;
  quad->nb_vertices = 4;
exit:
  return err;
error:
  if(quad->rbi)
    RBU(geometry_ref_put(quad));
  memset(quad, 0, sizeof(struct rbu_geometry));
  goto exit;
}

EXPORT_SYM int
rbu_init_circle
  (const struct rbi* rbi,
   struct rb_context* ctxt,
   unsigned int npoints,
   float x,
   float y,
   float radius,
   struct rbu_geometry* circle)
{
  struct rb_buffer_desc buf_desc;
  struct rb_buffer_attrib buf_attrib;
  int err = 0;
  memset(&buf_desc, 0, sizeof(buf_desc));

  if(UNLIKELY(circle == NULL || rbi == NULL)) {
    err = -1;
    goto error;
  }
  if(npoints > 1024) {
    err = -1;
    goto error;
  }

  init_geometry(rbi, ctxt, circle);
  {
    const float rcp_npoints = 1.f / (float)npoints;
    float vertices[npoints * 2];
    unsigned int point_id = 0;
    size_t coord_id = 0;
    memset(vertices, 0, sizeof(vertices));

    /* fill vertex data. */
    coord_id = 0;
    for(point_id = 0; point_id < npoints; ++point_id) {
      const float angle = (float)point_id * rcp_npoints * 2 * PI;
      vertices[coord_id] = x + cosf(angle) * radius;
      ++coord_id;
      vertices[coord_id] = y + sinf(angle) * radius;
      ++coord_id;
    }

    #define CALL(func) \
      do { \
        if(0 != (err = func)) \
        goto error; \
      } while(0)
    /* Create the vertex buffer. */
    buf_desc.size = sizeof(vertices);
    buf_desc.target = RB_BIND_VERTEX_BUFFER;
    buf_desc.usage = RB_USAGE_IMMUTABLE;
    CALL(rbi->create_buffer(ctxt, &buf_desc, vertices, &circle->vertex_buffer));
    /* Create the vertex array. */
    buf_attrib.index = 0;
    buf_attrib.stride = 2 * sizeof(float);
    buf_attrib.offset = 0;
    buf_attrib.type = RB_FLOAT2;
    CALL(rbi->create_vertex_array(ctxt, &circle->vertex_array));
    CALL(rbi->vertex_attrib_array
      (circle->vertex_array, circle->vertex_buffer, 1, &buf_attrib));
    #undef CALL

    circle->primitive_type = RB_LINE_LOOP;
    circle->nb_vertices = npoints;
  }
exit:
  return err;
error:
  if(circle->rbi)
    RBU(geometry_ref_put(circle));
  memset(circle, 0, sizeof(struct rbu_geometry));
  goto exit;
}

/*******************************************************************************
 *
 * Geometry functions.
 *
 ******************************************************************************/
EXPORT_SYM int
rbu_geometry_ref_get(struct rbu_geometry* geom)
{
  if(UNLIKELY(geom == NULL))
    return -1;
  ref_get(&geom->ref);
  return 0;
}

EXPORT_SYM int
rbu_geometry_ref_put(struct rbu_geometry* geom)
{
  if(UNLIKELY(geom == NULL))
    return -1;
  ref_put(&geom->ref, release_geometry);
  return 0;
}

EXPORT_SYM int
rbu_draw_geometry(struct rbu_geometry* geom)
{
  if(UNLIKELY(geom == NULL || geom->ctxt == NULL || geom->vertex_array == NULL))
    return -1;
  RBI(geom->rbi, bind_vertex_array(geom->ctxt, geom->vertex_array));
  RBI(geom->rbi, draw(geom->ctxt, geom->primitive_type, geom->nb_vertices));
  RBI(geom->rbi, bind_vertex_array(geom->ctxt, NULL));
  return 0;
}

