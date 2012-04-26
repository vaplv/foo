#ifndef APP_COMMAND_H
#define APP_COMMAND_H

#include "app/core/app_error.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct app;

enum app_cmdarg_type {
  APP_CMDARG_INT,
  APP_CMDARG_FLOAT,
  APP_CMDARG_STRING,
  APP_CMDARG_LITERAL,
  APP_CMDARG_FILE,
  APP_NB_CMDARG_TYPES
};

struct app_cmdarg_desc {
  enum app_cmdarg_type type;
  const char* short_options;
  const char* long_options;
  const char* data_type;
  const char* glossary;
  unsigned int min_count;
  unsigned int max_count;
  union app_cmdarg_domain {
    struct { int min, max; } integer;
    struct { float min, max; } real;
    struct { const char** value_list; } string;
  } domain;
};

/* Mark the end of cmdarg desc list declaration. */
static const struct app_cmdarg_desc APP_CMDARG_END = {
  .type = APP_NB_CMDARG_TYPES,
  .short_options = (void*)0xDEADBEEF,
  .long_options = (void*)0xDEADBEEF,
  .data_type = (void*)0xDEADBEEF,
  .glossary = (void*)0xDEADBEEF,
  .min_count = 1,
  .max_count = 0,
  .domain = { .integer = { .min = 1, .max = 0 } }
};

struct app_cmdarg {
  enum app_cmdarg_type type;
  size_t count;
  struct app_cmdarg_value {
    /* Used to define if an optionnal arg is setup by the command or not.
     * Required arguments are always defined. */
    bool is_defined;
    union {
      float real;
      int integer;
      const char* string; /* Valid for string, and file arg types. */
    } data;
  } value_list[];
};

/*******************************************************************************
 *
 * Helper Macros.
 *
 ******************************************************************************/
/* Private macro. */
#define APP_COMMON_CMDARG_SETUP__(t, sopt, lopt, data, glos, min, max) \
  .type = t, \
  .short_options = sopt, \
  .long_options = lopt, \
  .data_type = data, \
  .glossary = glos, \
  .min_count = min, \
  .max_count = max

#define APP_CMDARGV(...) \
  (struct app_cmdarg_desc[]){__VA_ARGS__}

#define APP_CMDARG_APPEND_FLOAT( \
  sopt, lopt, data, glos, min_count, max_count, min_val, max_val) { \
    APP_COMMON_CMDARG_SETUP__ \
      (APP_CMDARG_FLOAT, sopt, lopt, data, glos, min_count, max_count), \
    .domain = { .real = { .min = min_val, .max = max_val }} \
  }

#define APP_CMDARG_APPEND_INT( \
  sopt, lopt, data, glos, min_count, max_count, min_val, max_val) { \
    APP_COMMON_CMDARG_SETUP__ \
      (APP_CMDARG_INT, sopt, lopt, data, glos, min_count, max_count), \
    .domain = { .integer = { .min = min_val, .max = max_val }} \
  }

#define APP_CMDARG_APPEND_STRING( \
  sopt, lopt, data, glos, min_count, max_count, list) { \
    APP_COMMON_CMDARG_SETUP__ \
      (APP_CMDARG_STRING, sopt, lopt, data, glos, min_count, max_count), \
    .domain={ .string = { .value_list = list }} \
  }

#define APP_CMDARG_APPEND_LITERAL(sopt, lopt, glos, min, max) { \
    APP_COMMON_CMDARG_SETUP__ \
      (APP_CMDARG_LITERAL, (sopt), (lopt), NULL, (glos), (min), (max)), \
    .domain={ .string = { .value_list = NULL }} \
  }

#define APP_CMDARG_APPEND_FILE(sopt, lopt, data, glos, min_count, max_count) {\
    APP_COMMON_CMDARG_SETUP__ \
      (APP_CMDARG_FILE, sopt, lopt, data, glos, min_count, max_count), \
    .domain={ .string = { .value_list = NULL }} \
  }

/*******************************************************************************
 *
 * Command function prototypes.
 *
 ******************************************************************************/
extern enum app_error
app_add_command
  (struct app* app,
   const char* name,
   void (*func)(struct app*, size_t argc, const struct app_cmdarg** argv),
   enum app_error (*arg_completion) /* May be NULL.*/
    (struct app*, const char*, size_t, size_t*, const char**[]),
   const struct app_cmdarg_desc argv_desc[],
   const char* desc); /* May be NULL. */

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

extern enum app_error
app_man_command
  (struct app* app,
   const char* command,
   size_t* len, /* May be NULL. */
   size_t max_buf_len,
   char* buffer); /* May be NULL. */

extern enum app_error
app_command_arg_completion
  (struct app* app,
   const char* cmd_name,
   const char* arg_str,
   size_t arg_str_len,
   size_t* completion_list_len,
   const char** completion_list[]);

/* The returned list is valid until the add/del command function is called. */
extern enum app_error
app_command_name_completion
  (struct app* app,
   const char* cmd_name,
   size_t cmd_name_len,
   size_t* completion_list_len,
   const char** completion_list[]);

#endif /* APP_COMMAND2_H */

