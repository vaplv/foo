#ifndef EDIT_GTK_EXPLORER_H
#define EDIT_GTK_EXPLORER_H

#include "app/editor/edit_error.h"

struct app;
struct edit;

extern enum edit_error
edit_init_explorer
   (struct edit* edit);

extern enum edit_error
edit_shutdown_explorer
  (struct edit* edit);

#endif /* EDIT_GTK_EXPLORER_H */

