#ifndef RSRC_CONTEXT_C_H
#define RSRC_CONTEXT_C_H

#include "resources/rsrc_error.h"
#include "sys/ref_count.h"
#include "sys/sys.h"

#define RSRC_ERRBUF_LEN 1024

struct rsrc_font_library;

struct rsrc_context {
  struct ref ref;
  struct mem_allocator* allocator;
  struct rsrc_font_library* font_lib;
  char errbuf[RSRC_ERRBUF_LEN];
  size_t errbuf_id;
};

LOCAL_SYM enum rsrc_error
rsrc_print_error
  (struct rsrc_context* ctxt,
   const char* fmt,
   ...) FORMAT_PRINTF(2, 3);

#endif /* RSRC_CONTEXT_C_H */

