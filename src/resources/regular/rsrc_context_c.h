#ifndef RSRC_CONTEXT_C_H
#define RSRC_CONTEXT_C_H

#ifndef NDEBUG
  #include <assert.h>
  #define SL(func) assert(SL_NO_ERROR == sl_##func)
#else
  #define SL(func) sl_##func
#endif

struct sl_context;

struct rsrc_context {
  struct mem_allocator* allocator;
};

#endif /* RSRC_CONTEXT_C_H */

