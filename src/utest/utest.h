#ifndef UTEST_H
#define UTEST_H

#include <stdio.h>
#include <stdlib.h>

#define ERROR \
  do { \
    fprintf(stderr, "error:%s:%d\n", __FILE__, __LINE__); \
    exit(-1); \
  } while(0)

#define CHECK(a, b) do { if((a) != (b)) ERROR; } while(0)
#define NCHECK(a, b) do { if((a) == (b)) ERROR; } while(0)

#endif /* UTEST_H */

