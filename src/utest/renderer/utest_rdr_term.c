#include "renderer/rdr_font.h"
#include "renderer/rdr_system.h"
#include "renderer/rdr_term.h"
#include "resources/rsrc_context.h"
#include "resources/rsrc_font.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"
#include <wchar.h>

#define BAD_ARG RDR_INVALID_ARGUMENT
#define OK RDR_NO_ERROR

int
main(int argc, char** argv)
{
  wchar_t buffer[BUFSIZ];
  const char* driver_name = NULL;
  size_t i = 0;
  int err = 0;

  /* Window manager data structures. */
  struct wm_device* device = NULL;
  struct wm_window* window = NULL;
  struct wm_window_desc win_desc = {
    .width = 800, .height = 600, .fullscreen = false
  };
  /* Renderer data structures. */
  struct rdr_font* font = NULL;
  struct rdr_system* sys = NULL;
  struct rdr_term* term = NULL;

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    goto error;
  }
  driver_name = argv[1];

  CHECK(wm_create_device(NULL, &device), WM_NO_ERROR);
  CHECK(wm_create_window(device, &win_desc, &window), WM_NO_ERROR);
  CHECK(rdr_create_system(driver_name, NULL, &sys), OK);
  CHECK(rdr_create_font(sys, &font), OK);

  CHECK(rdr_create_term(NULL, NULL, 0, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 0, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 0, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 0, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 16, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 16, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 16, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 16, 0, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 0, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 0, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 0, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 0, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 16, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 16, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 16, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 16, 25, NULL), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 0, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 0, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 0, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 0, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 16, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 16, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 16, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 16, 0, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 0, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 0, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 0, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 0, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, NULL, 16, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, NULL, 16, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(NULL, font, 16, 25, &term), BAD_ARG);
  CHECK(rdr_create_term(sys, font, 16, 25, &term), OK);

  CHECK(rdr_term_font(NULL, NULL), BAD_ARG);
  CHECK(rdr_term_font(term, NULL), BAD_ARG);
  CHECK(rdr_term_font(NULL, font), BAD_ARG);
  CHECK(rdr_term_font(term, font), OK);

  CHECK(rdr_term_print(NULL, NULL), BAD_ARG);
  CHECK(rdr_term_print(term, NULL), BAD_ARG);
  CHECK(rdr_term_print(NULL, L"pouet"), BAD_ARG);
  CHECK(rdr_term_print(term, L"pouet"), OK);

  CHECK(rdr_term_dump(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_term_dump(term, NULL, NULL), OK);
  CHECK(rdr_term_dump(term, &i, NULL), OK);
  CHECK(i, 6);
  CHECK(i <= BUFSIZ, true);
  CHECK(rdr_term_dump(term, NULL, buffer), OK);
  CHECK(wcscmp(buffer, L"pouet"), 0);

  CHECK(rdr_term_print(term, L" pouet"), OK);
  CHECK(rdr_term_dump(term, &i, NULL), OK);
  CHECK(i <= BUFSIZ, true);
  CHECK(rdr_term_dump(term, &i, buffer), OK);
  CHECK(i, 12);
  CHECK(wcscmp(buffer, L"pouet pouet"), 0);

  CHECK(rdr_term_print(term, L"\ntest\n"), OK);
  CHECK(rdr_term_dump(term, &i, NULL), OK);
  CHECK(i <= BUFSIZ, true);
  CHECK(rdr_term_dump(term, &i, buffer), OK);
  CHECK(i, 18);
  CHECK(wcscmp(buffer, L"pouet pouet\ntest\n"), 0);

  CHECK(rdr_term_print(term, L"abcdefghijklmnopqrstuvwxyz\n"), OK);
  CHECK(rdr_term_dump(term, &i, NULL), OK);
  CHECK(i <= BUFSIZ, true);
  CHECK(i, 46);
  CHECK(rdr_term_dump(term, &i, buffer), OK);
  CHECK(i, 46);
  CHECK(wcscmp(buffer, L"pouet pouet\ntest\nabcdefghijklmnop\nqrstuvwxyz\n"), 0);
 
  CHECK(rdr_term_ref_get(NULL), BAD_ARG);
  CHECK(rdr_term_ref_get(term), OK);
  CHECK(rdr_term_ref_put(NULL), BAD_ARG);
  CHECK(rdr_term_ref_put(term), OK);
  CHECK(rdr_term_ref_put(term), OK);

  CHECK(rdr_font_ref_put(font), OK);
  CHECK(rdr_system_ref_put(sys), OK);
  CHECK(wm_window_ref_put(window), WM_NO_ERROR);
  CHECK(wm_device_ref_put(device), WM_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

exit:
  return err;
error:
  err = -1;
  goto exit;
}
