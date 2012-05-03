#include "renderer/rdr_system.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"

#define BAD_ARG RDR_INVALID_ARGUMENT
#define OK RDR_NO_ERROR

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

  CHECK(rdr_create_system(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_create_system(NULL, NULL, &sys), BAD_ARG);
  CHECK(rdr_create_system("__INVALID_DRIVER__", NULL, NULL), BAD_ARG);
  CHECK(rdr_create_system("__INVALID_DRIVER__", NULL, &sys), RDR_DRIVER_ERROR);
  CHECK(rdr_create_system(driver_name, NULL, &sys), OK);
  CHECK(rdr_system_ref_put(NULL), BAD_ARG);
  CHECK(rdr_system_ref_put(sys), OK);

  CHECK(wm_window_ref_put(window), WM_NO_ERROR);
  CHECK(wm_device_ref_put(device), WM_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;
}
