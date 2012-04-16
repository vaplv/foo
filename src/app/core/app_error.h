#ifndef APP_ERROR_H
#define APP_ERROR_H

enum app_error {
  APP_ALIGNMENT_ERROR,
  APP_COMMAND_ERROR,
  APP_INTERNAL_ERROR,
  APP_INVALID_ARGUMENT,
  APP_INVALID_FILE,
  APP_IO_ERROR,
  APP_MEMORY_ERROR,
  APP_NO_ERROR,
  APP_RENDER_DRIVER_ERROR,
  APP_UNKNOWN_ERROR
};

extern const char*
app_error_string
  (enum app_error err);

#endif /* APP_ERROR_H */

