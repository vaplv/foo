#include "render_backend/rbi.h"
#include "render_backend/rbu.h"
#include "sys/sys.h"
#include <assert.h>
#include <string.h>

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
release_quad(struct ref* ref)
{
  struct rbu_quad* quad = NULL;
  assert(ref);

  quad = CONTAINER_OF(ref, struct rbu_quad, ref);
  if(LIKELY(quad->rbi != NULL)) {
    if(LIKELY(quad->vertex_buffer != NULL))
      RBI(quad->rbi, buffer_ref_put(quad->vertex_buffer));
    if(LIKELY(quad->vertex_array != NULL))
      RBI(quad->rbi, vertex_array_ref_put(quad->vertex_array));

    if(LIKELY(quad->ctxt != NULL))
      RBI(quad->rbi, context_ref_put(quad->ctxt));
    memset(quad, 0, sizeof(struct rbu_quad));
  }
}

/*******************************************************************************
 *
 * Quad functions.
 *
 ******************************************************************************/
EXPORT_SYM int
rbu_init_quad
  (const struct rbi* rbi,
   struct rb_context* ctxt,
   float x, float y, float w, float h,
   struct rbu_quad* quad)
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
  memset(quad, 0, sizeof(struct rbu_quad));
  ref_init(&quad->ref);
  RBI(rbi, context_ref_get(ctxt));
  quad->ctxt = ctxt;
  quad->rbi = rbi;

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
exit:
  return err;
error:
  if(quad->rbi)
    RBU(quad_ref_put(quad));
  memset(quad, 0, sizeof(struct rbu_quad));
  goto exit;
}

EXPORT_SYM int
rbu_quad_ref_get(struct rbu_quad* quad)
{
  if(UNLIKELY(quad == NULL))
    return -1;
  ref_get(&quad->ref);
  return 0;
}

EXPORT_SYM int
rbu_quad_ref_put(struct rbu_quad* quad)
{
  if(UNLIKELY(quad == NULL))
    return -1;
  ref_put(&quad->ref, release_quad);
  return 0;
}

EXPORT_SYM int
rbu_draw_quad(struct rbu_quad* quad)
{
  if(UNLIKELY(quad == NULL || quad->ctxt == NULL || quad->vertex_array == NULL))
    return -1;
  RBI(quad->rbi, bind_vertex_array(quad->ctxt, quad->vertex_array));
  RBI(quad->rbi, draw(quad->ctxt, RB_TRIANGLE_STRIP, 4));
  RBI(quad->rbi, bind_vertex_array(quad->ctxt, NULL));
  return 0;
}

