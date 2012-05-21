#include "renderer/rdr_system.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"
#include <stdio.h>
#include <string.h>

#define BAD_ARG RDR_INVALID_ARGUMENT
#define OK RDR_NO_ERROR

STATIC_ASSERT(sizeof(void*) >= 4, Unexpected_pointer_size);

static void 
stream_func0(const char* msg, void* data)
{
  int* i = data;
  CHECK(i, NULL);
  printf("logger0: %s", msg);
}

static void 
stream_func1(const char* msg, void* data)
{
  int* i = data;
  CHECK((uintptr_t)i, 0xDEADBEEF);
  printf("logger1: %s", msg);
}

int
main(int argc, char** argv)
{
  const char* driver_name = NULL;
  FILE* file = NULL;
  void* ptr = (void*)0xDEADBEEF;
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

  CHECK(rdr_system_attach_log_stream(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_system_attach_log_stream(sys, NULL, NULL), BAD_ARG);
  CHECK(rdr_system_attach_log_stream(NULL, stream_func0, NULL), BAD_ARG);
  CHECK(rdr_system_attach_log_stream(sys, stream_func0, NULL), OK);
  CHECK(rdr_system_attach_log_stream(NULL, NULL, ptr), BAD_ARG);
  CHECK(rdr_system_attach_log_stream(sys, NULL, ptr), BAD_ARG);
  CHECK(rdr_system_attach_log_stream(NULL, stream_func1, ptr), BAD_ARG);
  CHECK(rdr_system_attach_log_stream(sys, stream_func1, ptr), OK);

  CHECK(rdr_system_detach_log_stream(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_system_detach_log_stream(sys, NULL, NULL), BAD_ARG);
  CHECK(rdr_system_detach_log_stream(NULL, stream_func0, NULL), BAD_ARG);
  CHECK(rdr_system_detach_log_stream(sys, stream_func1, NULL), BAD_ARG);
  CHECK(rdr_system_detach_log_stream(sys, stream_func0, NULL), OK);
  CHECK(rdr_system_detach_log_stream(sys, stream_func0, NULL), BAD_ARG);
  CHECK(rdr_system_detach_log_stream(NULL, NULL, ptr), BAD_ARG);
  CHECK(rdr_system_detach_log_stream(sys, NULL, ptr), BAD_ARG);
  CHECK(rdr_system_detach_log_stream(NULL, stream_func1, ptr), BAD_ARG);
  CHECK(rdr_system_detach_log_stream(sys, stream_func0, ptr), BAD_ARG);
  CHECK(rdr_system_detach_log_stream(sys, stream_func1, ptr), OK);
  CHECK(rdr_system_detach_log_stream(sys, stream_func1, ptr), BAD_ARG);

  CHECK(rdr_system_attach_log_stream(sys, stream_func1, ptr), OK);

  CHECK(rdr_system_ref_get(NULL), BAD_ARG);
  CHECK(rdr_system_ref_get(sys), OK);
  CHECK(rdr_system_ref_put(NULL), BAD_ARG);
  CHECK(rdr_system_ref_put(sys), OK);
  CHECK(rdr_system_ref_put(sys), OK);

  CHECK(wm_window_ref_put(window), WM_NO_ERROR);
  CHECK(wm_device_ref_put(device), WM_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;
}
