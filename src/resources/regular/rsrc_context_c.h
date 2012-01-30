#ifndef RSRC_CONTEXT_C_H
#define RSRC_CONTEXT_C_H

#include "sys/ref_count.h"

#ifndef NDEBUG
  #include <assert.h>
  #define SL(func) assert(SL_NO_ERROR == sl_##func)
#else
  #define SL(func) sl_##func
#endif

struct sl_context;
struct rsrc_font_library;

struct rsrc_context {
  struct ref ref;
  struct mem_allocator* allocator;
  struct rsrc_font_library* font_lib; 
};

#endif /* RSRC_CONTEXT_C_H */

