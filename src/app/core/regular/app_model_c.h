#ifndef APP_MODEL_C_H
#define APP_MODEL_C_H

#include "sys/sys.h"

struct app_model;
struct app_object;

LOCAL_SYM struct app_model*
app_object_to_model
  (struct app_object* obj);

#endif /* APP_MODEL_C_H */

