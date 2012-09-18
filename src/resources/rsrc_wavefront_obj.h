#ifndef RSRC_WAVEFRONT_OBJ_H
#define RSRC_WAVEFRONT_OBJ_H

#include "resources/rsrc.h"
#include "resources/rsrc_error.h"

struct rsrc_context;
struct rsrc_wavefront_obj;

RSRC_API enum rsrc_error
rsrc_create_wavefront_obj
  (struct rsrc_context* ctxt,
   struct rsrc_wavefront_obj** out_wobj);

RSRC_API enum rsrc_error
rsrc_wavefront_obj_ref_get
  (struct rsrc_wavefront_obj* wobj);

RSRC_API enum rsrc_error
rsrc_wavefront_obj_ref_put
  (struct rsrc_wavefront_obj* wobj);

RSRC_API enum rsrc_error
rsrc_load_wavefront_obj
  (struct rsrc_wavefront_obj* wobj,
   const char* path);

#endif /* RSRC_WAVEFRONT_OBJ_H */

