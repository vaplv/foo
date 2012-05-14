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
    if(geom->index_buffer != NULL)
      RBI(geom->rbi, buffer_ref_put(geom->index_buffer));
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
  struct rb_buffer_attrib buf_attr;
  int err = 0;
  memset(&buf_desc, 0, sizeof(buf_desc));
  memset(&buf_attr, 0, sizeof(buf_attr));

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
  buf_attr.index = 0;
  buf_attr.stride = 2 * sizeof(float);
  buf_attr.offset = 0;
  buf_attr.type = RB_FLOAT2;
  CALL(rbi->create_vertex_array(ctxt, &quad->vertex_array));
  CALL(rbi->vertex_attrib_array
    (quad->vertex_array, quad->vertex_buffer, 1, &buf_attr));
  #undef CALL

  quad->primitive_type = RB_TRIANGLE_STRIP;
  quad->nb_vertices = 4;
exit:
  return err;
error:
  if(quad) {
    if(quad->rbi)
      RBU(geometry_ref_put(quad));
    memset(quad, 0, sizeof(struct rbu_geometry));
  }
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
  struct rb_buffer_attrib buf_attr;
  int err = 0;
  memset(&buf_desc, 0, sizeof(buf_desc));
  memset(&buf_attr, 0, sizeof(buf_attr));

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
    buf_attr.index = 0;
    buf_attr.stride = 2 * sizeof(float);
    buf_attr.offset = 0;
    buf_attr.type = RB_FLOAT2;
    CALL(rbi->create_vertex_array(ctxt, &circle->vertex_array));
    CALL(rbi->vertex_attrib_array
      (circle->vertex_array, circle->vertex_buffer, 1, &buf_attr));
    #undef CALL

    circle->primitive_type = RB_LINE_LOOP;
    circle->nb_vertices = npoints;
  }
exit:
  return err;
error:
  if(circle) {
    if(circle->rbi)
      RBU(geometry_ref_put(circle));
    memset(circle, 0, sizeof(struct rbu_geometry));
  }
  goto exit;
}

EXPORT_SYM int
rbu_init_parallelepiped
  (const struct rbi* rbi,
   struct rb_context* ctxt,
   float pos[3],
   float size[3],
   bool wireframe,
   struct rbu_geometry* paral)
{
  struct rb_buffer_desc buf_desc;
  struct rb_buffer_attrib buf_attr;
  float vertices[24];
  float minpt[3] = {0.f, 0.f, 0.f};
  float maxpt[3] = {0.f, 0.f, 0.f};
  float hsize[3] = {0.f, 0.f, 0.f};
  int err = 0;
  memset(vertices, 0, sizeof(vertices));
  memset(&buf_desc, 0, sizeof(buf_desc));
  memset(&buf_attr, 0, sizeof(buf_attr));

  if(UNLIKELY(rbi==NULL || pos==NULL || size==NULL || paral==NULL)) {
    err = -1;
    goto error;
  }
  init_geometry(rbi, ctxt, paral);
  /* Define vertex positions.
   *     7+----+6
   *     /|   /|
   *   4+----+5|
   *    |3+--|-+2
   *    |/   |/
   *   0+----+1    */
  hsize[0] = size[0] * 0.5f;
  hsize[1] = size[1] * 0.5f;
  hsize[2] = size[2] * 0.5f;
  minpt[0]=pos[0]-hsize[0]; minpt[1]=pos[1]-hsize[1]; minpt[2]=pos[2]-hsize[2];
  maxpt[0]=pos[0]+hsize[0]; maxpt[1]=pos[1]+hsize[1]; maxpt[2]=pos[2]+hsize[2];
  vertices[0] = minpt[0]; vertices[1] = minpt[1]; vertices[2] = maxpt[2];
  vertices[3] = maxpt[0]; vertices[4] = minpt[1]; vertices[5] = maxpt[2];
  vertices[6] = maxpt[0]; vertices[7] = minpt[1]; vertices[8] = minpt[2];
  vertices[9] = minpt[0]; vertices[10]= minpt[1]; vertices[11]= minpt[2];
  vertices[12]= minpt[0]; vertices[13]= maxpt[1]; vertices[14]= maxpt[2];
  vertices[15]= maxpt[0]; vertices[16]= maxpt[1]; vertices[17]= maxpt[2];
  vertices[18]= maxpt[0]; vertices[19]= maxpt[1]; vertices[20]= minpt[2];
  vertices[21]= minpt[0]; vertices[22]= maxpt[1]; vertices[23]= minpt[2];

  #define CALL(func) if(0 != (err = func)) goto error
  #define SETUP_INDEX_BUFFER(indices, prim_type) \
    do { \
      buf_desc.size = sizeof(indices); \
      buf_desc.target = RB_BIND_INDEX_BUFFER; \
      buf_desc.usage = RB_USAGE_IMMUTABLE; \
      CALL(rbi->create_buffer \
        (ctxt, &buf_desc, indices, &paral->index_buffer)); \
      paral->primitive_type = prim_type; \
      paral->nb_vertices = sizeof(indices) / sizeof(unsigned int); \
    } while(0)

  /* Create the vertex buffer. */
  buf_desc.size = sizeof(vertices);
  buf_desc.target = RB_BIND_VERTEX_BUFFER;
  buf_desc.usage = RB_USAGE_IMMUTABLE;
  CALL(rbi->create_buffer(ctxt, &buf_desc, vertices, &paral->vertex_buffer));
  /* Create the index buffer. */
  if(wireframe) {
    const unsigned int indices[16] = {
      0, 4, 5, 1, 2, 6, 7, 3, 0, 4, 7, 3, 2, 6, 5, 1
    };
    SETUP_INDEX_BUFFER(indices, RB_LINE_LOOP);
  } else {
    const unsigned int indices[14] = {
      0, 1, 3, 2, 6, 1, 5, 0, 4, 3, 7, 6, 4, 5
    };
    SETUP_INDEX_BUFFER(indices, RB_TRIANGLE_STRIP);
  }
  /* Create the vertex array. */
  buf_attr.index = 0;
  buf_attr.stride = 3 * sizeof(float);
  buf_attr.offset = 0;
  buf_attr.type = RB_FLOAT3;
  CALL(rbi->create_vertex_array(ctxt, &paral->vertex_array));
  CALL(rbi->vertex_index_array(paral->vertex_array, paral->index_buffer));
  CALL(rbi->vertex_attrib_array
    (paral->vertex_array, paral->vertex_buffer, 1, &buf_attr));

  #undef SETUP_INDEX_BUFFER
  #undef CALL
exit:
  return err;
error:
  if(paral) {
    if(paral->rbi)
      RBU(geometry_ref_put(paral));
    memset(paral, 0, sizeof(struct rbu_geometry));
  }
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

