#include "renderer/rdr_material.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_model.h"
#include "renderer/rdr_model_instance.h"
#include "renderer/rdr_system.h"
#include "renderer/rdr_world.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"
#include "sys/sys.h"
#include <math.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define M_DEG_TO_RAD(x) ((x) * 3.14159 / 180.f)
#define RDR(func) \
  do { \
    UNUSED const enum rdr_error rdr_err = rdr_##func; \
    assert(rdr_err == RDR_NO_ERROR); \
  } while(0)

static bool
is_driver_null(const char* name)
{
  const char* null_driver_name = "librbnull.so";
  char* p = NULL;

  if(name == NULL)
    return false;

  p = strstr(name, null_driver_name);

  return (p != NULL)
    && (p == name ? true : *(p-1) == '/')
    && (*(p + strlen(null_driver_name)) == '\0');
}

static void
compute_transform(float angle, float x, float y, float z, float* view)
{
  const float cosa = cosf(angle);
  const float sina = sinf(angle);

  view[0] = cosa;
  view[1] = 0;
  view[2] = -sina;
  view[3] = 0.f;

  view[4] = 0.f;
  view[5] = 1.f;
  view[6] = 0.f;
  view[7] = 0.f;

  view[8] = sina;
  view[9] = 0.f;
  view[10] = cosa;
  view[11] = 0.f;

  view[12] = x;
  view[13] = y;
  view[14] = z;
  view[15] = 1.f;
}

static int
create_mesh(struct rdr_system* sys, struct rdr_mesh** out_mesh)
{
  /* Vertex buffer. Interleaved positions (float3) and colors (float3). */
  const float vertices[] = {
    -1.f, -1.f, -1.f, 0.f, 0.f, 0.f,
    -1.f, 1.f, -1.f, 0.f, 1.f, 0.f,
    1.f, 1.f, -1.f, 1.f, 1.f, 0.f,
    1.f, -1.f, -1.f, 1.f, 0.f, 0.f,
    -1.f, -1.f, 1.f, 0.f, 0.f, 1.f,
    -1.f, 1.f, 1.f, 0.f, 1.f, 1.f,
    1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
    1.f, -1.f, 1.f, 1.f, 0.f, 1.f
  };
  /* Indices of the cube. */
  const unsigned int indices[] = {
    0, 1, 2, 0, 2, 3, /* back */
    3, 2, 6, 3, 6, 7, /* right */
    7, 6, 5, 7, 5, 4, /* front */
    4, 5, 1, 4, 1, 0, /* left */
    4, 0, 7, 0, 3, 7, /* bottom */
    5, 2, 1, 5, 6, 2  /* top */
  };
  /* Mesh description. */
  const struct rdr_mesh_attrib attrib_list[] = {
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT3 },
    { .usage = RDR_ATTRIB_COLOR, .type = RDR_FLOAT3 }
  };
  struct rdr_mesh* mesh = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  rdr_err = rdr_create_mesh(sys, &mesh);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = rdr_mesh_data
    (sys, mesh, 2, attrib_list, sizeof(vertices), vertices);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = rdr_mesh_indices
    (sys, mesh, sizeof(indices)/sizeof(unsigned int), indices);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

exit:
  *out_mesh = mesh;
  return err;

error:
  if(mesh) {
    rdr_free_mesh(sys, mesh);
    mesh = NULL;
  }
  err = -1;
  goto exit;
}

static int
create_material(struct rdr_system* sys, struct rdr_material** out_material)
{
  const char* vs_source =
    "#version 330\n"
    "uniform mat4x4 rdr_modelview;\n"
    "uniform mat4x4 rdr_projection;\n"
    "uniform vec3 color_bias;\n"
    "in vec3 rdr_position;\n"
    "in vec3 rdr_color;\n"
    "smooth out vec3 theColor;\n"
    "void main()\n"
    "{\n"
    " theColor = fract(rdr_color * color_bias);\n"
    " gl_Position = rdr_projection * rdr_modelview * vec4(rdr_position, 1.f);\n"
    "}\n";
  const char* fs_source =
    "#version 330\n"
    "smooth in vec3 theColor;\n"
    "out vec4 color;\n"
    "void main()\n"
    "{\n"
    " color = vec4(theColor, 1.f);\n"
    "}\n";
  const char* shader_sources[RDR_NB_SHADER_USAGES] = {
    [RDR_VERTEX_SHADER] = vs_source,
    [RDR_FRAGMENT_SHADER] = fs_source,
    [RDR_GEOMETRY_SHADER] = NULL
  };
  struct rdr_material* mtr = NULL;
  enum rdr_error rdr_err = RDR_NO_ERROR;
  int err = 0;

  rdr_err = rdr_create_material(sys, &mtr);
  if(rdr_err != RDR_NO_ERROR)
    goto error;

  rdr_err = rdr_material_program(sys, mtr, shader_sources);
  if(rdr_err != RDR_NO_ERROR) {
    const char* log = NULL;
    rdr_err = rdr_get_material_log(sys, mtr, &log);
    if(rdr_err != RDR_NO_ERROR)
      goto error;
    if(log)
      fprintf(stderr, "Material program error: \n%s\n", log);
    goto error;
  }

exit:
  *out_material = mtr;
  return err;

error:
  if(mtr) {
    rdr_free_material(sys, mtr);
    mtr = NULL;
  }
  err = -1;
  goto exit;
}

int
main(int argc, char* argv[])
{
  /* Window manager data structures. */
  struct wm_device* device = NULL;
  struct wm_window* window = NULL;
  const struct wm_window_desc win_desc = {
    .width = 800,
    .height = 600,
    .fullscreen = 0
  };

  /* Renderer data structure. */
  struct rdr_system* sys = NULL;
  struct rdr_mesh* mesh = NULL;
  struct rdr_material* mtr = NULL;
  struct rdr_model* model = NULL;
  struct rdr_view view;
  struct rdr_world* world = NULL;
  struct rdr_model_instance* instance_list[27];

  /* Miscellaneous data. */
  const char* driver_name = NULL;
  float angle = 0.f;
  float transform_matrix[16];
  enum wm_error wm_err = WM_NO_ERROR;
  int err = 0;
  int i = 0;
  bool null_driver = false;

  memset(instance_list, 0, sizeof(struct rdr_model_instance*) * 27);

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    goto error;
  }
  driver_name = argv[1];

  if(rdr_create_system(driver_name, NULL, &sys) != RDR_NO_ERROR) {
    fprintf(stderr, "Invalid render backend driver: %s\n",  driver_name);
    goto error;
  }
  null_driver = is_driver_null(driver_name);

  wm_err = wm_create_device(NULL, &device);
  if(wm_err != WM_NO_ERROR) {
    fprintf(stderr, "Error creating the window manager device.\n");
    goto error;
  }

  wm_err = wm_create_window(device, &win_desc, &window);
  if(wm_err != WM_NO_ERROR) {
    fprintf(stderr, "Error spawning a window.\n");
    goto error;
  }

  err = create_mesh(sys, &mesh);
  if(err != 0) {
    fprintf(stderr, "Error creating the mesh.\n");
    goto error;
  }

  err = create_material(sys, &mtr);
  if(err != 0) {
    fprintf(stderr, "Error creating the material.\n");
    goto error;
  }

  RDR(create_model(sys, mesh, mtr, &model));
  RDR(create_world(sys, &world));
  RDR(background_color(sys, world, (float[]){0.1f, 0.1f, 0.1f}));

  for(i = 0; i < 27; ++i) {
    RDR(create_model_instance(sys, model, &instance_list[i]));
    RDR(add_model_instance(sys, world, instance_list[i]));

    if(null_driver == false) {
      struct rdr_model_instance_data uniform;
      size_t nb = 0;
      RDR(get_model_instance_uniforms(sys, instance_list[i], &nb, &uniform));
      ((float*)uniform.data)[0] = rand() / (float)RAND_MAX;
      ((float*)uniform.data)[1] = rand() / (float)RAND_MAX;
      ((float*)uniform.data)[2] = rand() / (float)RAND_MAX;
    }
  }

  view.proj_ratio = (float)win_desc.width / (float)win_desc.height;
  view.fov_x = M_DEG_TO_RAD(70.f);
  view.znear = 1.f;
  view.zfar = 100.f;
  view.x = 0;
  view.y = 0;
  view.width = win_desc.width;
  view.height = win_desc.height;
  memset(view.transform, 0, sizeof(float) * 16);

  view.transform[0] =
  view.transform[5] =
  view.transform[10] =
  view.transform[15] = 1.f;

  while(1) {
    int x, y, z;
    for(z = 0; z < 3; ++z) {
      for(y = 0; y < 3; ++y) {
        for(x = 0; x < 3; ++x) {
          const float pos[3] = {
            [0] = (x-1) * 3.5f,
            [1] = (y-1) * 3.5f,
            [2] = (z-1) * 3.5f
          };
          compute_transform(0.f, pos[0], pos[1], pos[2], transform_matrix);
          RDR(model_instance_transform
              (sys, instance_list[z*9 + y*3 + x], transform_matrix));
        }
      }
    }

    compute_transform(M_DEG_TO_RAD(angle), 0.f, 0.f, -20.f, view.transform);
    RDR(draw_world(sys, world, &view));

    if(wm_swap(device, window) != WM_NO_ERROR) {
      fprintf(stderr, "Error swaping the window.\n");
      goto error;
    }

    angle += M_DEG_TO_RAD(0.5f);
  }

exit:
  if(world)
    RDR(free_world(sys, world));
  for(i = 0; i < 27; ++i) {
    if(instance_list[i])
      RDR(free_model_instance(sys, instance_list[i]));
  }
  if(model)
    RDR(free_model(sys, model));
  if(mesh)
    RDR(free_mesh(sys, mesh));
  if(mtr)
    RDR(free_material(sys, mtr));
  if(sys)
    RDR(free_system(sys));
  if(window) {
    wm_err = wm_free_window(device, window);
    assert(wm_err == WM_NO_ERROR);
  }
  if(device) {
    wm_err = wm_free_device(device);
    assert(wm_err == WM_NO_ERROR);
  }
  return err;

error:
  err = -1;
  goto exit;
}

