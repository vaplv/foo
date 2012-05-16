#include "renderer/rdr_mesh.h"
#include "renderer/rdr_system.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"
#include <float.h>
#include <math.h>

#define BAD_ARG RDR_INVALID_ARGUMENT
#define OK RDR_NO_ERROR
#define SZ sizeof

#define CHECK_EPS(a, b) \
  CHECK(fabsf(a - b) < 1.e-7f, true)

int
main(int argc, char** argv)
{
  const char* driver_name = NULL;
  FILE* file = NULL;
  struct rdr_system* sys = NULL;
  struct wm_device* device = NULL;
  struct wm_window* window = NULL;
  struct wm_window_desc win_desc = {
    .width = 800, .height = 600, .fullscreen = 0
  };
  struct rdr_mesh* mesh = NULL;
  const float vert1[] = { 0.f, 0.f, 0.f };
  const float vert2[] = { 0.f, 0.f, 0.f, 1.f, 1.f, 1.f };
  const float vert3[] = {
    0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f
  };
  const float vert4[] = {
    0.f, 5.f, 7.f, 1.f, 10.f, -20.f, 6.f, 0.f
  };
  const unsigned int indices1[] = { 0, 1, 2 };
  const unsigned int indices2[] = { 0, 1, 2, 3 };
  const struct rdr_mesh_attrib mesh_attr[] = {
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT2 },
    { .usage = RDR_ATTRIB_COLOR, .type = RDR_FLOAT },
    { .usage = RDR_ATTRIB_UNKNOWN, .type = RDR_FLOAT }
  };
  const struct rdr_mesh_attrib mesh_attr2[] = {
    { .usage = RDR_ATTRIB_NORMAL, .type = RDR_UNKNOWN_TYPE }
  };
  const struct rdr_mesh_attrib mesh_attr3[] = {
    { .usage = RDR_ATTRIB_UNKNOWN, .type = RDR_FLOAT }
  };
  const struct rdr_mesh_attrib mesh_attr4[] = {
    { .usage = RDR_ATTRIB_COLOR, .type = RDR_FLOAT3 },
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT3 }
  };
  const struct rdr_mesh_attrib mesh_attr5[] = {
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT4 }
  };
  float min_bound[3] = {0.f, 0.f, 0.f};
  float max_bound[3] = {0.f, 0.f, 0.f};

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    return -1;
  }
  driver_name = argv[1];
  file = fopen(driver_name, "r");
  if(!file) {
    fprintf(stderr, "Invalid driver %s\n", driver_name);
    return -1;
  }
  fclose(file);

  CHECK(wm_create_device(NULL, &device), WM_NO_ERROR);
  CHECK(wm_create_window(device, &win_desc, &window), WM_NO_ERROR);
  CHECK(rdr_create_system(driver_name, NULL, &sys), OK);

  CHECK(rdr_create_mesh(NULL, NULL), BAD_ARG);
  CHECK(rdr_create_mesh(NULL, &mesh), BAD_ARG);
  CHECK(rdr_create_mesh(sys, NULL), BAD_ARG);
  CHECK(rdr_create_mesh(sys, &mesh), OK);

  CHECK(rdr_get_mesh_aabb(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_mesh_aabb(mesh, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_mesh_aabb(NULL, min_bound, NULL), BAD_ARG);
  CHECK(rdr_get_mesh_aabb(mesh, min_bound, NULL), BAD_ARG);
  CHECK(rdr_get_mesh_aabb(NULL, NULL, max_bound), BAD_ARG);
  CHECK(rdr_get_mesh_aabb(mesh, NULL, max_bound), BAD_ARG);
  CHECK(rdr_get_mesh_aabb(NULL, min_bound, max_bound), BAD_ARG);
  CHECK(rdr_get_mesh_aabb(mesh, min_bound, max_bound), OK);
  CHECK(min_bound[0] > max_bound[0], true);
  CHECK(min_bound[1] > max_bound[1], true);
  CHECK(min_bound[2] > max_bound[2], true);

  CHECK(rdr_mesh_data(NULL, 0, NULL, 0, NULL), BAD_ARG);
  CHECK(rdr_mesh_data(NULL, 1, NULL, 0, NULL), BAD_ARG);
  CHECK(rdr_mesh_data(NULL, 0, mesh_attr, 0, NULL), BAD_ARG);
  CHECK(rdr_mesh_data(NULL, 0, NULL, 0, NULL), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 0, mesh_attr, SZ(vert1), vert1), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 2, NULL, SZ(vert1), vert1), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 2, mesh_attr, 0, vert1), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 2, mesh_attr, SZ(vert1), NULL), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 2, NULL, SZ(vert1), NULL), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 1, mesh_attr, SZ(vert1), vert1), BAD_ARG);
  CHECK(rdr_mesh_data(NULL, 2, mesh_attr, SZ(vert1), vert1), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 1, mesh_attr2, SZ(vert1), vert1), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 1, mesh_attr3, SZ(vert1), vert1), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 0, mesh_attr, 0, vert1), OK);
  CHECK(rdr_get_mesh_aabb(mesh, min_bound, max_bound), OK);
  CHECK(min_bound[0] > max_bound[0], true);
  CHECK(min_bound[1] > max_bound[1], true);
  CHECK(min_bound[2] > max_bound[2], true);

  CHECK(rdr_mesh_data(mesh, 0, NULL, 0, NULL), OK);
  CHECK(rdr_get_mesh_aabb(mesh, min_bound, max_bound), OK);
  CHECK(min_bound[0] > max_bound[0], true);
  CHECK(min_bound[1] > max_bound[1], true);
  CHECK(min_bound[2] > max_bound[2], true);
  CHECK(rdr_mesh_data(mesh, 2, mesh_attr, SZ(vert1), vert1), OK);
  CHECK(rdr_get_mesh_aabb(mesh, min_bound, max_bound), OK);
  CHECK(min_bound[0], 0.f);
  CHECK(min_bound[1], 0.f);
  CHECK(min_bound[2], 0.f);
  CHECK(max_bound[0], 0.f);
  CHECK(max_bound[1], 0.f);
  CHECK(max_bound[2], 0.f);
  CHECK(rdr_mesh_data(mesh, 2, mesh_attr, SZ(vert2), vert2), OK);
  CHECK(rdr_get_mesh_aabb(mesh, min_bound, max_bound), OK);
  CHECK(min_bound[0], 0.f);
  CHECK(min_bound[1], 0.f);
  CHECK(min_bound[2], 0.f);
  CHECK(max_bound[0], 1.f);
  CHECK(max_bound[1], 1.f);
  CHECK(max_bound[2], 0.f);
  CHECK(rdr_mesh_data(mesh, 1, mesh_attr4 + 1, SZ(vert3), vert3), OK);
  CHECK(rdr_get_mesh_aabb(mesh, min_bound, max_bound), OK);
  CHECK(min_bound[0], 0.f);
  CHECK(min_bound[1], 1.f);
  CHECK(min_bound[2], 2.f);
  CHECK(max_bound[0], 9.f);
  CHECK(max_bound[1], 10.f);
  CHECK(max_bound[2], 11.f);
  CHECK(rdr_mesh_data(mesh, 1, mesh_attr4, SZ(vert3), vert3), OK);
  CHECK(rdr_get_mesh_aabb(mesh, min_bound, max_bound), OK);
  CHECK(min_bound[0] > max_bound[0], true);
  CHECK(min_bound[1] > max_bound[1], true);
  CHECK(min_bound[2] > max_bound[2], true);
  CHECK(rdr_mesh_data(mesh, 2, mesh_attr4, SZ(vert3), vert3), OK);
  CHECK(rdr_get_mesh_aabb(mesh, min_bound, max_bound), OK);
  CHECK(min_bound[0], 3.f);
  CHECK(min_bound[1], 4.f);
  CHECK(min_bound[2], 5.f);
  CHECK(max_bound[0], 9.f);
  CHECK(max_bound[1], 10.f);
  CHECK(max_bound[2], 11.f);
  CHECK(rdr_mesh_data(mesh, 1, mesh_attr5, SZ(vert3), vert3), OK);
  CHECK(rdr_get_mesh_aabb(mesh, min_bound, max_bound), OK);
  CHECK(min_bound[0], 0.f);
  CHECK_EPS(min_bound[1], 1.f/3.f);
  CHECK_EPS(min_bound[2], 2.f/3.f);
  CHECK_EPS(max_bound[0], 8.f/11.f);
  CHECK_EPS(max_bound[1], 9.f/11.f);
  CHECK_EPS(max_bound[2], 10.f/11.f);
  CHECK(rdr_mesh_data(mesh, 1, mesh_attr5, SZ(vert4), vert4), OK);
  CHECK(rdr_get_mesh_aabb(mesh, min_bound, max_bound), OK);
  CHECK(min_bound[0], -FLT_MAX);
  CHECK(min_bound[1], -FLT_MAX);
  CHECK(min_bound[2], -FLT_MAX);
  CHECK(max_bound[0], FLT_MAX);
  CHECK(max_bound[1], FLT_MAX);
  CHECK(max_bound[2], FLT_MAX);
  CHECK(rdr_mesh_data(mesh, 1, mesh_attr, SZ(vert2), vert2), OK);
  CHECK(rdr_get_mesh_aabb(mesh, min_bound, max_bound), OK);
  CHECK(min_bound[0], 0.f);
  CHECK(min_bound[1], 0.f);
  CHECK(min_bound[2], 0.f);
  CHECK(max_bound[0], 1.f);
  CHECK(max_bound[1], 1.f);
  CHECK(max_bound[2], 0.f);

  CHECK(rdr_mesh_indices(NULL, 3, NULL), BAD_ARG);
  CHECK(rdr_mesh_indices(mesh, 4, indices2), BAD_ARG);
  CHECK(rdr_mesh_indices(mesh, 3, indices1), OK);
  CHECK(rdr_mesh_indices(mesh, 0, indices1), OK);

  CHECK(rdr_mesh_ref_get(NULL), BAD_ARG);
  CHECK(rdr_mesh_ref_get(mesh), OK);
  CHECK(rdr_mesh_ref_put(NULL), BAD_ARG);
  CHECK(rdr_mesh_ref_put(mesh), OK);
  CHECK(rdr_mesh_ref_put(mesh), OK);
  CHECK(rdr_system_ref_put(sys), OK);

  CHECK(wm_window_ref_put(window), WM_NO_ERROR);
  CHECK(wm_device_ref_put(device), WM_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;
}

