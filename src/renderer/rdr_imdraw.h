#ifndef RDR_IMDRAW_H
#define RDR_IMDRAW_H

#include "sys/sys.h"

enum rdr_imdraw_flag {
  RDR_IMDRAW_FLAG_UPPERMOST_LAYER = BIT(0),
  RDR_IMDRAW_FLAG_FIXED_SCREEN_SIZE = BIT(1),
  RDR_IMDRAW_FLAG_NONE = 0
};

#endif /* RDR_IMDRAW_H */

