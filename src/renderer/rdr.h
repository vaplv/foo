#ifndef RDR_H
#define RDR_H

#ifndef NDEBUG
  #include <assert.h>
  #define RDR(func) assert(RDR_NO_ERROR == rdr_##func)
#else
  #define RDR(func) rdr_##func
#endif

#endif /* RDR_H */
