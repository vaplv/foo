#include "renderer/rdr_material.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_model.h"
#include "renderer/rdr_model_instance.h"
#include "renderer/rdr_system.h"
#include "renderer/rdr_world.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int
main(int argc, char** argv)
{
  const char* driver_name = NULL;
  FILE* file = NULL;
  int err = 0;

  /* Window manager data structures. */
  struct wm_device* device = NULL;
  struct wm_window* window = NULL;
  struct wm_window_desc win_desc = {
    .width = 800, .height = 600, .fullscreen = false
  };

  /* Renderer data structures. */
  struct rdr_system* sys = NULL;
  struct rdr_mesh* mesh = NULL;
  struct rdr_material* mtr = NULL;
  struct rdr_model* mdl = NULL;
  struct rdr_model_instance* inst0 = NULL;
  struct rdr_model_instance* inst1 = NULL;
  struct rdr_model_instance* inst2 = NULL;
  struct rdr_world* world = NULL;

  /* Renderer data. */
  const float data[] = { 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f };
  const unsigned int indices[] = { 0, 1, 0 };
  const struct rdr_mesh_attrib attr[] = {
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT4 }
  };
  const char* sources[RDR_NB_SHADER_USAGES] = {
    [RDR_VERTEX_SHADER] =
      "#version 330\n"
      "in vec4 rdr_position;\n"
      "void main()\n"
      "{\n"
        "gl_Position = rdr_position;\n"
      "}\n",
    [RDR_GEOMETRY_SHADER] = NULL,
    [RDR_FRAGMENT_SHADER] = NULL
  };
  const struct rdr_view view = {
    .transform = {
      1.f, 0.f, 0.f, 0.f,
      0.f, 1.f, 0.f, 0.f,
      0.f, 0.f, 1.f, 0.f,
      0.f, 0.f, 0.f, 1.f
    },
    .proj_ratio = win_desc.width / win_desc.height,
    .fov_x = 1.4f,
    .znear = 1.f,
    .zfar = 100.f,
    .x = 0,
    .y = 0,
    .width = win_desc.width,
    .height = win_desc.height
  };

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    goto error;
  }
  driver_name = argv[1];

  file = fopen(driver_name, "r");
  if(!file) {
    fprintf(stderr, "Invalid driver %s\n", driver_name);
    goto error;
  }
  fclose(file);

  CHECK(wm_create_device(NULL, &device), WM_NO_ERROR);
  CHECK(wm_create_window(device, &win_desc, &window), WM_NO_ERROR);

  CHECK(rdr_create_system(driver_name, NULL, &sys), RDR_NO_ERROR);

  CHECK(rdr_create_mesh(sys, &mesh), RDR_NO_ERROR);
  CHECK(rdr_mesh_data(mesh, 1, attr, sizeof(data), data), RDR_NO_ERROR);
  CHECK(rdr_mesh_indices(mesh, 3, indices), RDR_NO_ERROR);
  CHECK(rdr_create_material(sys, &mtr), RDR_NO_ERROR);
  CHECK(rdr_material_program(mtr, sources), RDR_NO_ERROR);
  CHECK(rdr_create_model(sys, mesh, mtr, &mdl), RDR_NO_ERROR);
  CHECK(rdr_create_model_instance(sys, mdl, &inst0), RDR_NO_ERROR);
  CHECK(rdr_create_model_instance(sys, mdl, &inst1), RDR_NO_ERROR);
  CHECK(rdr_create_model_instance(sys, mdl, &inst2), RDR_NO_ERROR);

  CHECK(rdr_create_world(NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_world(sys, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_world(NULL, &world), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_world(sys, &world), RDR_NO_ERROR);

  CHECK(rdr_add_model_instance(NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_add_model_instance(world, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_add_model_instance(NULL, inst0), RDR_INVALID_ARGUMENT);
  CHECK(rdr_add_model_instance(world, inst0), RDR_NO_ERROR);

  CHECK(rdr_draw_world(NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_draw_world(world, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_draw_world(NULL, &view), RDR_INVALID_ARGUMENT);
  CHECK(rdr_draw_world(world, &view), RDR_NO_ERROR);

  CHECK(rdr_add_model_instance(world, inst1), RDR_NO_ERROR);
  CHECK(rdr_add_model_instance(world, inst2), RDR_NO_ERROR);

  CHECK(rdr_remove_model_instance(NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_remove_model_instance(world, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_remove_model_instance(NULL, inst0), RDR_INVALID_ARGUMENT);
  CHECK(rdr_remove_model_instance(world, inst0), RDR_NO_ERROR);
  CHECK(rdr_remove_model_instance(world, inst0), RDR_INVALID_ARGUMENT);
  CHECK(rdr_remove_model_instance(world, inst1), RDR_NO_ERROR);
  CHECK(rdr_remove_model_instance(world, inst2), RDR_NO_ERROR);

  CHECK(rdr_world_ref_get(NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_world_ref_get(world), RDR_NO_ERROR);
  CHECK(rdr_world_ref_put(NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_world_ref_put(world), RDR_NO_ERROR);
  CHECK(rdr_world_ref_put(world), RDR_NO_ERROR);

  CHECK(rdr_model_instance_ref_put(inst0), RDR_NO_ERROR);
  CHECK(rdr_model_instance_ref_put(inst1), RDR_NO_ERROR);
  CHECK(rdr_model_instance_ref_put(inst2), RDR_NO_ERROR);
  CHECK(rdr_model_ref_put(mdl), RDR_NO_ERROR);
  CHECK(rdr_material_ref_put(mtr), RDR_NO_ERROR);
  CHECK(rdr_mesh_ref_put(mesh), RDR_NO_ERROR);
  CHECK(rdr_system_ref_put(sys), RDR_NO_ERROR);

  CHECK(wm_free_window(device, window), WM_NO_ERROR);
  CHECK(wm_free_device(device), WM_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

exit:
  return err;

error:
  err = -1;
  goto exit;
}
