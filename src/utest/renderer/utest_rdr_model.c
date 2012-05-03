#include "renderer/rdr_material.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_model.h"
#include "renderer/rdr_system.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"
#include <stdio.h>
#include <string.h>

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
  struct rdr_material* mtr = NULL;
  struct rdr_mesh* mesh = NULL;
  struct rdr_model* model0 = NULL;
  struct rdr_model* model1 = NULL;
  const char* vs_source =
    "#version 330\n"
    "void main()\n"
    "{\n"
      "gl_Position = vec4(0.f,0.f,0.f,0.f);\n"
    "}\n";
  const char* sources[RDR_NB_SHADER_USAGES];
  const float data[] = { 0.f, 0.f, 0.f, 1.f, 1.f, 1.f };
  const struct rdr_mesh_attrib attr0[] = {
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT2 },
    { .usage = RDR_ATTRIB_COLOR, .type = RDR_FLOAT }
  };
  const struct rdr_mesh_attrib attr1[] = {
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT2 }
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

  memset(sources, 0, SZ(const char*) * RDR_NB_SHADER_USAGES);
  sources[RDR_VERTEX_SHADER] = vs_source;

  CHECK(wm_create_device(NULL, &device), WM_NO_ERROR);
  CHECK(wm_create_window(device, &win_desc, &window), WM_NO_ERROR);

  CHECK(rdr_create_system(driver_name, NULL, &sys), OK);
  CHECK(rdr_create_mesh(sys, &mesh), OK);
  CHECK(rdr_mesh_data(mesh, 2, attr0, SZ(data), data), OK);
  CHECK(rdr_create_material(sys, &mtr), OK);
  CHECK(rdr_material_program(mtr, sources), OK);

  CHECK(rdr_create_model(NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_create_model(sys, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_create_model(NULL, mesh, NULL, NULL), BAD_ARG);
  CHECK(rdr_create_model(sys, mesh, NULL, NULL), BAD_ARG);

  CHECK(rdr_create_model(NULL, NULL, mtr, NULL), BAD_ARG);
  CHECK(rdr_create_model(sys, NULL, mtr, NULL), BAD_ARG);
  CHECK(rdr_create_model(NULL, mesh, mtr, NULL), BAD_ARG);
  CHECK(rdr_create_model(sys, mesh, mtr, NULL), BAD_ARG);

  CHECK(rdr_create_model(NULL, NULL, mtr, &model0), BAD_ARG);
  CHECK(rdr_create_model(sys, NULL, mtr, &model0), BAD_ARG);
  CHECK(rdr_create_model(NULL, mesh, mtr, &model0), BAD_ARG);
  CHECK(rdr_create_model(sys, mesh, mtr, &model0), OK);
  CHECK(rdr_create_model(sys, mesh, mtr, &model1), OK);

  CHECK(rdr_model_mesh(NULL, NULL), BAD_ARG);
  CHECK(rdr_model_mesh(model0, NULL), BAD_ARG);
  CHECK(rdr_model_mesh(NULL, mesh), BAD_ARG);
  CHECK(rdr_model_mesh(model0, mesh), OK);

  CHECK(rdr_mesh_data(mesh, 1, attr1, SZ(data), data), OK);

  CHECK(rdr_model_ref_get(NULL), BAD_ARG);
  CHECK(rdr_model_ref_get(model0), OK);

  CHECK(rdr_model_ref_put(NULL), BAD_ARG);
  CHECK(rdr_model_ref_put(model0), OK);
  CHECK(rdr_model_ref_put(model0), OK);
  CHECK(rdr_model_ref_put(model1), OK);

  CHECK(rdr_mesh_ref_put(mesh), OK);
  CHECK(rdr_material_ref_put(mtr), OK);
  CHECK(rdr_system_ref_put(sys), OK);

  CHECK(wm_window_ref_put(window), WM_NO_ERROR);
  CHECK(wm_device_ref_put(device), WM_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}

