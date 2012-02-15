#ifndef RSRC_CONTEXT_C_H
#define RSRC_CONTEXT_C_H

#include "sys/ref_count.h"

struct sl_context;
struct rsrc_font_library;

struct rsrc_context {
  struct ref ref;
  struct mem_allocator* allocator;
  struct rsrc_font_library* font_lib; 
};

#endif /* RSRC_CONTEXT_C_H */

