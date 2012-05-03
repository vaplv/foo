#include "renderer/rdr_mesh.h"
#include "renderer/rdr_system.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"

#define BAD_ARG RDR_INVALID_ARGUMENT
#define OK RDR_NO_ERROR
#define SZ sizeof

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
  CHECK(rdr_mesh_data(mesh, 0, NULL, 0, NULL), OK);
  CHECK(rdr_mesh_data(mesh, 2, mesh_attr, SZ(vert1), vert1), OK);
  CHECK(rdr_mesh_data(mesh, 2, mesh_attr, SZ(vert2), vert2), OK);
  CHECK(rdr_mesh_data(mesh, 1, mesh_attr, SZ(vert2), vert2), OK);

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

