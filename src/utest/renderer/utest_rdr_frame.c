#include "renderer/rdr_font.h"
#include "renderer/rdr_frame.h"
#include "renderer/rdr_system.h"
#include "renderer/rdr_term.h"
#include "renderer/rdr_world.h"
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
  int err = 0;
  /* Window manager data structures. */
  struct wm_device* device = NULL;
  struct wm_window* window = NULL;
  struct wm_window_desc win_desc = {
    .width = 800, .height = 600, .fullscreen = false
  };
  struct rdr_frame_desc frame_desc = {
    .width = win_desc.width, .height = win_desc.height
  };
  /* Renderer data structure. */
  struct rdr_frame* frame = NULL;
  struct rdr_system* sys = NULL;
  struct rdr_world* world = NULL;
  struct rdr_font* font = NULL;
  struct rdr_term* term = NULL;
  const unsigned int pos[2] = { 0, 0 };
  const unsigned int size[2] = { 1, 1 };
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

  CHECK(wm_create_device(NULL, &device), WM_NO_ERROR);
  CHECK(wm_create_window(device, &win_desc, &window), WM_NO_ERROR);
  CHECK(rdr_create_system(driver_name, NULL, &sys), OK);
  CHECK(rdr_create_world(sys, &world), OK);
  CHECK(rdr_create_font(sys, &font), OK);
  CHECK(rdr_create_term(sys, font, 16, 25, &term), OK);

  CHECK(rdr_create_frame(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_create_frame(sys, NULL, NULL), BAD_ARG);
  CHECK(rdr_create_frame(NULL, NULL, &frame), BAD_ARG);
  CHECK(rdr_create_frame(sys, NULL, &frame), BAD_ARG);
  CHECK(rdr_create_frame(NULL, &frame_desc, NULL), BAD_ARG);
  CHECK(rdr_create_frame(sys, &frame_desc, NULL), BAD_ARG);
  CHECK(rdr_create_frame(NULL, &frame_desc, &frame), BAD_ARG);
  CHECK(rdr_create_frame(sys, &frame_desc, &frame), OK);

  CHECK(rdr_background_color(NULL, NULL), BAD_ARG);
  CHECK(rdr_background_color(frame, NULL), BAD_ARG);
  CHECK(rdr_background_color(NULL, (float[]){0.1f, 0.2f, 0.3f}), BAD_ARG);
  CHECK(rdr_background_color(frame, (float[]){0.1f, 0.2f, 0.3f}), OK);

  CHECK(rdr_frame_draw_world(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_frame_draw_world(frame, NULL, NULL), BAD_ARG);
  CHECK(rdr_frame_draw_world(NULL, world, NULL), BAD_ARG);
  CHECK(rdr_frame_draw_world(frame, world, NULL), BAD_ARG);
  CHECK(rdr_frame_draw_world(NULL, NULL, &view), BAD_ARG);
  CHECK(rdr_frame_draw_world(frame, NULL, &view), BAD_ARG);
  CHECK(rdr_frame_draw_world(NULL, world, &view), BAD_ARG);
  CHECK(rdr_frame_draw_world(frame, world, &view), OK);

  CHECK(rdr_frame_draw_term(NULL, NULL), BAD_ARG);
  CHECK(rdr_frame_draw_term(frame, NULL), BAD_ARG);
  CHECK(rdr_frame_draw_term(NULL, term), BAD_ARG);
  CHECK(rdr_frame_draw_term(frame, term), OK);

  CHECK(rdr_frame_pick_model_instance(NULL, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, world, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, world, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, NULL, &view, NULL, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, NULL, &view, NULL, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, world, &view, NULL, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, world,&view, NULL, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, NULL, NULL, pos, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, NULL, NULL, pos, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, world, NULL, pos, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, world, NULL, pos, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, NULL, &view, pos, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, NULL, &view, pos, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, world, &view, pos, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, world, &view, pos, NULL), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, NULL, NULL, NULL, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, NULL, NULL, NULL, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, world, NULL, NULL, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, world, NULL, NULL, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, NULL, &view, NULL, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, NULL, &view, NULL, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, world, &view, NULL, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, world, &view, NULL, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, NULL, NULL, pos, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, NULL, NULL, pos, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, world, NULL, pos, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, world, NULL, pos, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, NULL, &view, pos, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, NULL, &view, pos, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(NULL, world, &view, pos, size), BAD_ARG);
  CHECK(rdr_frame_pick_model_instance(frame, world, &view, pos, size), OK);

  CHECK(rdr_flush_frame(NULL), BAD_ARG);
  CHECK(rdr_flush_frame(frame), OK);

  CHECK(rdr_frame_ref_get(NULL), BAD_ARG);
  CHECK(rdr_frame_ref_get(frame), OK);
  CHECK(rdr_frame_ref_put(NULL), BAD_ARG);
  CHECK(rdr_frame_ref_put(frame), OK);
  CHECK(rdr_frame_ref_put(frame), OK);

  /* Check that the release of the frame is correctly handled despite it was
   * not flushed. */
  CHECK(rdr_create_frame(sys, &frame_desc, &frame), OK);
  CHECK(rdr_frame_draw_world(frame, world, &view), OK);
  CHECK(rdr_frame_draw_term(frame, term), OK);
  CHECK(rdr_frame_pick_model_instance(frame, world, &view, pos, size), OK);
  CHECK(rdr_frame_ref_put(frame), OK);

  CHECK(rdr_system_ref_put(sys), OK);
  CHECK(rdr_world_ref_put(world), OK);
  CHECK(rdr_font_ref_put(font), OK);
  CHECK(rdr_term_ref_put(term), OK);
  CHECK(wm_window_ref_put(window), WM_NO_ERROR);
  CHECK(wm_device_ref_put(device), WM_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

exit:
  return err;
error:
  err = -1;
  goto exit;
}

