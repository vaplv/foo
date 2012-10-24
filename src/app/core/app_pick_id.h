#ifndef APP_PICK_ID_H
#define APP_PICK_ID_H

#include <stdint.h>

#define APP_PICK_ID_MAX ((1 << 24) - 1)

#define APP_PICK_NONE \
  (app_pick_t)APP_PICK(APP_PICK_ID_MAX, APP_PICK_GROUP_NONE)

#define APP_PICK_GROUP_GET(pick) \
  (((pick) >> 24) & 0xFF)

#define APP_PICK_ID_GET(pick) \
  ((pick) & 0x00FFFFFF)

#define APP_PICK(pick, group) \
  (((pick) & 0x00FFFFFF) | (((group) & 0xFF) << 24))

enum app_pick_group {
  APP_PICK_GROUP_WORLD,
  APP_PICK_GROUP_IMDRAW,
  APP_PICK_GROUP_NONE = 0xFF
};

typedef uint32_t app_pick_t;

#endif /* APP_PICK_ID_H */
