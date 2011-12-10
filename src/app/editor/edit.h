#ifndef EDIT_H
#define EDIT_H

#include "app/editor/edit_error.h"
#include <stdbool.h>

struct app;
struct edit;
struct mem_allocator;

enum edit_error
edit_init
  (struct app* app,
   const char* gui_desc,
   struct mem_allocator* allocator, /* May be NULL. */
   struct edit** edit);

enum edit_error
edit_shutdown
  (struct edit* edit);

enum edit_error
edit_run
  (struct edit* edit,
   bool* keep_running); 

#endif /* EDIT_H */

