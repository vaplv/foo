#ifndef UTEST_H
#define UTEST_H

#include <stdio.h>
#include <stdlib.h>

#define ERROR \
  do { \
    fprintf(stderr, "error:%s:%d\n", __FILE__, __LINE__); \
    exit(-1); \
  } while(0)

#define CHECK(a, b) if((a) != (b)) ERROR
#define NCHECK(a, b) if((a) == (b)) ERROR

#endif /* UTEST_H */

