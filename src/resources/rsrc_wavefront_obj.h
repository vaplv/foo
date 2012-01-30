#ifndef RSRC_WAVEFRONT_OBJ_H
#define RSRC_WAVEFRONT_OBJ_H

#include "resources/rsrc_error.h"

struct rsrc_context;
struct rsrc_wavefront_obj;

extern enum rsrc_error
rsrc_create_wavefront_obj
  (struct rsrc_context* ctxt,
   struct rsrc_wavefront_obj** out_wobj);

extern enum rsrc_error
rsrc_free_wavefront_obj
  (struct rsrc_wavefront_obj* wobj);

extern enum rsrc_error
rsrc_load_wavefront_obj
  (struct rsrc_wavefront_obj* wobj,
   const char* path);

#endif /* RSRC_WAVEFRONT_OBJ_H */

