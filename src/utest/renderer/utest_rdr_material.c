#include "renderer/rdr_material.h"
#include "renderer/rdr_system.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"
#include <stdbool.h>
#include <string.h>

#define BAD_ARG RDR_INVALID_ARGUMENT
#define OK RDR_NO_ERROR
#define SZ sizeof

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
  const char* bad_source = "__INVALID_SOURCE_";
  const char* good_source =
    "#version 330\n"
    "void main()\n"
    "{\n"
      "gl_Position = vec4(0.f,0.f,0.f,0.f);\n"
    "}\n";
  const char* log = NULL;
  const char* sources[RDR_NB_SHADER_USAGES];
  bool null_driver = false;
  memset(sources, 0, SZ(const char*) * RDR_NB_SHADER_USAGES);

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
  null_driver = is_driver_null(driver_name);

  CHECK(wm_create_device(NULL, &device), WM_NO_ERROR);
  CHECK(wm_create_window(device, &win_desc, &window), WM_NO_ERROR);

  CHECK(rdr_create_system(driver_name, NULL, &sys), OK);

  CHECK(rdr_create_material(NULL, NULL), BAD_ARG);
  CHECK(rdr_create_material(sys, NULL), BAD_ARG);
  CHECK(rdr_create_material(NULL, &mtr), BAD_ARG);
  CHECK(rdr_create_material(sys, &mtr), OK);

  sources[RDR_VERTEX_SHADER] = bad_source;
  CHECK(rdr_material_program(NULL, NULL), BAD_ARG);
  CHECK(rdr_material_program(mtr, NULL), BAD_ARG);
  CHECK(rdr_material_program(NULL, sources), BAD_ARG);
  if(null_driver == false)
    CHECK(rdr_material_program(mtr, sources), RDR_DRIVER_ERROR);

  CHECK(rdr_get_material_log(NULL, NULL), BAD_ARG);
  CHECK(rdr_get_material_log(mtr, NULL), BAD_ARG);
  CHECK(rdr_get_material_log(NULL, &log), BAD_ARG);
  CHECK(rdr_get_material_log(mtr, &log), OK);

  sources[RDR_VERTEX_SHADER] = good_source;
  CHECK(rdr_material_program(mtr, sources), OK);

  CHECK(rdr_material_ref_get(NULL), BAD_ARG);
  CHECK(rdr_material_ref_get(mtr), OK);
  CHECK(rdr_material_ref_put(NULL), BAD_ARG);
  CHECK(rdr_material_ref_put(mtr), OK);
  CHECK(rdr_material_ref_put(mtr), OK);

  CHECK(rdr_system_ref_put(sys), OK);
  CHECK(wm_window_ref_put(window), WM_NO_ERROR);
  CHECK(wm_device_ref_put(device), WM_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;
}
