#ifndef APP_COMMAND_H
#define APP_COMMAND_H

#include "app/core/app_error.h"
#include <stddef.h>

#define APP_MAX_CMDARGS 16

enum app_cmdarg_type {
  APP_CMDARG_INT,
  APP_CMDARG_FLOAT,
  APP_CMDARG_STRING,
  APP_NB_CMDARG_TYPES
};

struct app_cmdarg_value_list {
  const char** buffer;
  size_t length;
};

struct app_cmdarg_desc {
  enum app_cmdarg_type type;
  union {
    struct { float min, max; } real;
    struct { int min, max;  } integer;
    struct { 
      /* Return the list of value that the string can have. */
      struct app_cmdarg_value_list 
        (*value_list)
          (struct app* app,
           const char* input, /* input to complete. May be NULL. */
           size_t input_len); 
    } string;
  } domain;
};

struct app_cmdarg {
  enum app_cmdarg_type type;
  union {
    float real;
    int integer;
    const char* string;
  } value;
};

struct app;

/*******************************************************************************
 *
 * Helper Macros.
 *
 ******************************************************************************/
#define APP_CMDARGV(...) \
  (struct app_cmdarg_desc[]){__VA_ARGS__}

#define APP_CMDARG_APPEND_FLOAT(min_val, max_val) \
  {.type = APP_CMDARG_FLOAT, .domain={.real={.min = min_val, .max = max_val}}}

#define APP_CMDARG_APPEND_INT(min_val, max_val) \
  {.type = APP_CMDARG_INT, .domain={.integer={.min = min_val, .max = max_val}}}

#define APP_CMDARG_APPEND_STRING(list) \
  {.type = APP_CMDARG_STRING, .domain={.string={.value_list=list}}}

#define APP_CMDARG_APPEND_FLOAT3(min_val, max_val) \
  APP_CMDARG_APPEND_FLOAT(min_val, max_val), \
  APP_CMDARG_APPEND_FLOAT(min_val, max_val), \
  APP_CMDARG_APPEND_FLOAT(min_val, max_val)

/*******************************************************************************
 *
 * Command function prototypes.
 *
 ******************************************************************************/
extern enum app_error
app_add_command
  (struct app* app,
   const char* name,
   void (*func)(struct app*, size_t argc, struct app_cmdarg* argv),
   size_t argc,
   const struct app_cmdarg_desc argv_desc[],
   const char* description);

extern enum app_error
app_del_command
  (struct app* app,
   const char* name);

extern enum app_error
app_has_command
  (struct app* app,
   const char* name,
   bool* has_command);

extern enum app_error
app_execute_command
  (struct app* app,
   const char* command);

/* The returned list is valid until the add/del command function is called. */
extern enum app_error
app_command_completion
  (struct app* app,
   const char* cmd_name,
   size_t cmd_name_len,
   size_t* completion_list_len,
   const char** completion_list[]); 

#endif /* APP_COMMAND_H */

