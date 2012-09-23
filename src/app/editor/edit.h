#ifndef EDIT_H
#define EDIT_H

#include "sys/sys.h"

#if defined(BUILD_EDIT)
  #define EDIT_API EXPORT_SYM
#else
  #define EDIT_API IMPORT_SYM
#endif

#endif /* EDIT_H */

