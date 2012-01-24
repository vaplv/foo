#ifndef RSRC_FONT_C_H
#define RSRC_FONT_C_H

#include "resources/rsrc_error.h"

struct rsrc_font_library;

extern enum rsrc_error
rsrc_init_font_library
  (struct rsrc_context* ctxt);

extern enum rsrc_error
rsrc_shutdown_font_library
  (struct rsrc_context* ctxt);

#endif /* RSRC_FONT_C_H */

