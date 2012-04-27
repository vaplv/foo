#include "resources/rsrc.h"
#include "resources/rsrc_context.h"
#include "resources/rsrc_geometry.h"
#include "resources/rsrc_wavefront_obj.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int
main(int argc, char** argv)
{
  struct timeval t0, t1;
  suseconds_t us;
  struct rsrc_context* ctxt = NULL;
  struct rsrc_wavefront_obj* wobj = NULL;
  struct rsrc_geometry* geom = NULL;
  const char* errstr = NULL;
  size_t nb_prim_set = 0;
  size_t prim_set_id = 0;
  size_t total_nb_vertices = 0;
  size_t total_nb_points = 0;
  size_t total_nb_lines = 0;
  size_t total_nb_triangles = 0;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  int err = 0;

  if(argc != 2) {
    printf("usage: %s WAVEFRONT_OBJ\n", argv[0]);
    goto error;
  }

  RSRC(create_context(NULL, &ctxt));
  RSRC(create_wavefront_obj(ctxt, &wobj));
  RSRC(create_geometry(ctxt, &geom));

  err = gettimeofday(&t0, NULL);
  assert(err == 0);
  RSRC(flush_error(ctxt));
  rsrc_err = rsrc_load_wavefront_obj(wobj, argv[1]);
  if(rsrc_err != RSRC_NO_ERROR) {
    fprintf(stderr, "Error loading the wavefront obj: %s\n", argv[1]);
    RSRC(get_error_string(ctxt, &errstr));
    if(errstr)
      fprintf(stderr, "%s", errstr);
    goto error;
  }
  err = gettimeofday(&t1, NULL);
  assert(err == 0);
  us = (t1.tv_sec * 1000000 + t1.tv_usec) - (t0.tv_sec * 1000000 + t0.tv_usec);
  printf("Loading wavefront obj: %.3f ms\n", us / 1000.f);

  err = gettimeofday(&t0, NULL);
  assert(err == 0);
  RSRC(flush_error(ctxt));
  rsrc_err = rsrc_geometry_from_wavefront_obj(geom, wobj);
  if(rsrc_err != RSRC_NO_ERROR) {
    fprintf(stderr, "Error creating the geometry from the wavefront obj.\n");
    RSRC(get_error_string(ctxt, &errstr));
    if(errstr)
      fprintf(stderr, "%s", errstr);
    goto error;
  }
  err = gettimeofday(&t1, NULL);
  assert(err == 0);
  us = (t1.tv_sec * 1000000 + t1.tv_usec) - (t0.tv_sec * 1000000 + t0.tv_usec);
  printf("Creating geometry: %.3f ms\n", us / 1000.f);

  RSRC(get_primitive_set_count(geom, &nb_prim_set));
  for(prim_set_id = 0; prim_set_id < nb_prim_set; ++prim_set_id) {
    struct rsrc_primitive_set set;
    const char* prim_name = NULL;
    size_t sizeof_vertex = 0;
    size_t i = 0;
    unsigned short indices_per_primitive;

    printf("Set %zu -- ", prim_set_id);
    RSRC(get_primitive_set(geom, prim_set_id, &set));
    switch(set.primitive_type) {
      case RSRC_POINT:
        prim_name = "Point";
        indices_per_primitive = 1;
        total_nb_points += set.nb_indices / indices_per_primitive;
        break;
      case RSRC_LINE:
        prim_name = "Line";
        indices_per_primitive = 2;
        total_nb_lines += set.nb_indices / indices_per_primitive;
        break;
      case RSRC_TRIANGLE:
        prim_name = "Triangle";
        indices_per_primitive = 3;
        total_nb_triangles += set.nb_indices/indices_per_primitive;
        break;
      default:
        indices_per_primitive = 0;
        assert(0);
        break;
    }
    assert(set.nb_indices % indices_per_primitive == 0);
    printf("%s count: %zu; ", prim_name, set.nb_indices/indices_per_primitive);

    for(i = sizeof_vertex = 0; i < set.nb_attribs; ++i) {
      switch(set.attrib_list[i].type) {
        case RSRC_FLOAT:
          sizeof_vertex += 1 * sizeof(float);
          break;
        case RSRC_FLOAT2:
          sizeof_vertex += 2 * sizeof(float);
          break;
        case RSRC_FLOAT3:
          sizeof_vertex += 3 * sizeof(float);
          break;
        case RSRC_FLOAT4:
          sizeof_vertex += 4 * sizeof(float);
          break;
        default:
          assert(0);
          break;
      }
    }
    assert(set.sizeof_data % sizeof_vertex == 0);
    printf("Vertex count: %zu\n", set.sizeof_data / sizeof_vertex);
    total_nb_vertices += set.sizeof_data / sizeof_vertex;
  }
  printf("Total vertex count: %zd\n", total_nb_vertices);
  printf("Total point count: %zd\n", total_nb_points);
  printf("Total line count: %zd\n", total_nb_lines);
  printf("Total triangle count: %zd\n", total_nb_triangles);

exit:
  if(geom)
    RSRC(geometry_ref_put(geom));
  if(wobj)
    RSRC(wavefront_obj_ref_put(wobj));
  if(ctxt)
    RSRC(context_ref_put(ctxt));

  return err;

error:
  err = -1;
  goto exit;
}

