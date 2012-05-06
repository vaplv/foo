#ifndef APP_CVAR_H
#define APP_CVAR_H

#include "app/core/app_error.h"
#include <stdbool.h>

enum app_cvar_type {
  APP_CVAR_BOOL,
  APP_CVAR_INT,
  APP_CVAR_STRING,
  APP_CVAR_FLOAT,
  APP_NB_CVAR_TYPES
};

struct app_cvar {
  enum app_cvar_type type;
  union app_cvar_value {
    bool boolean;
    float real;
    int integer;
    const char* string;
  } value;
};

struct app_cvar_desc {
  enum app_cvar_type type;
  union app_cvar_value init_value;
  union app_cvar_domain {
    struct { int min, max; } integer;
    struct { float min, max; } real;
    struct { const char** value_list; } string;
  } domain;
};

/*******************************************************************************
 *
 * Helper macros.
 *
 ******************************************************************************/
#define APP_CVAR_BOOL_DESC(init_val) \
  (struct app_cvar_desc[]){{ \
    .type = APP_CVAR_BOOL, \
    .init_value = { .boolean = init_val }, \
    .domain = { .integer = { .min = 1, .max = -1 }} \
  }}


#define APP_CVAR_INT_DESC(init_val, min_val, max_val) \
  (struct app_cvar_desc[]){{ \
    .type = APP_CVAR_INT, \
    .init_value = { .integer = init_val }, \
    .domain = { .integer = { .min = min_val, .max = max_val }} \
  }}

#define APP_CVAR_FLOAT_DESC(init_val, min_val, max_val) \
  (struct app_cvar_desc[]){{ \
    .type = APP_CVAR_FLOAT, \
    .init_value = { .real = init_val }, \
    .domain = { .real = { .min = min_val, .max = max_val }} \
  }}

#define APP_CVAR_STRING_DESC(init_val, val_list) \
  (struct app_cvar_desc[]){{ \
    .type = APP_CVAR_STRING, \
    .init_value = { .string = init_val }, \
    .domain = { .string = { .value_list = val_list }} \
  }}

#define APP_CVAR_BOOL_VALUE(val) (union app_cvar_value){ .boolean = val}
#define APP_CVAR_INT_VALUE(val) (union app_cvar_value){ .integer = val}
#define APP_CVAR_FLOAT_VALUE(val) (union app_cvar_value){ .real = val}
#define APP_CVAR_STRING_VALUE(val) (union app_cvar_value){ .string = val}

/*******************************************************************************
 *
 * Cvar function prototypes.
 *
 ******************************************************************************/
extern enum app_error
app_add_cvar
  (struct app* app,
   const char* name,
   const struct app_cvar_desc* desc);

extern enum app_error
app_del_cvar
  (struct app* app,
   const char* name);

extern enum app_error
app_set_cvar
  (struct app* app,
   const char* name,
   union app_cvar_value value);

extern enum app_error
app_get_cvar
  (struct app* app,
   const char* name,
   const struct app_cvar** cvar); /* Set to NULL if the cvar does not exist. */

#endif /* APP_CVAR_H */


