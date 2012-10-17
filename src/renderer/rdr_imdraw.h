#ifndef RDR_IMDRAW_H
#define RDR_IMDRAW_H

#include "sys/sys.h"

enum rdr_imdraw_flag {
  RDR_IMDRAW_FLAG_UPPERMOST_LAYER = BIT(0), /* Draw above the world */
  RDR_IMDRAW_FLAG_FIXED_SCREEN_SIZE = BIT(1), /* Fix the screen space size */
  RDR_IMDRAW_FLAG_NONE = 0
};

enum rdr_im_vector_marker {
  RDR_IM_VECTOR_CUBE_MARKER,
  RDR_IM_VECTOR_CONE_MARKER,
  RDR_IM_VECTOR_MARKER_NONE
};

#endif /* RDR_IMDRAW_H */

