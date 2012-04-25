cmake_minimum_required(VERSION 2.6)

find_path(GLFW_INC_PATH argtable2.h)
find_library(GLFW_LIB_PATH argtable2)

if(NOT GLFW_INC_PATH)
  message(FATAL_ERROR "Can't find GL/glfw.h.")
endif()

if(NOT GLFW_LIB_PATH)
  message(FATAL_ERROR "Can't find glfw.")
endif()


add_library(appcore SHARED 
  app.c
  app_builtin_commands.c
  app_command.c
  app_command_buffer.c
  app_error.c
  app_model.c
  app_model_instance.c
  app_object.c
  app_term.c
  app_view.c
  app_world.c)

target_link_libraries(appcore renderer rsrc sl sys wmglfw argtable2)
