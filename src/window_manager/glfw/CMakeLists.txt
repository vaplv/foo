cmake_minimum_required(VERSION 2.6)

find_path(GLFW_INC_PATH GL/glfw.h)
find_library(GLFW_LIB_PATH glfw)

if(NOT GLFW_INC_PATH)
  message(FATAL_ERROR "Can't find GL/glfw.h.")
endif()

if(NOT GLFW_LIB_PATH)
  message(FATAL_ERROR "Can't find glfw.")
endif()

add_library(wmglfw SHARED 
  wm_glfw_device.c
  wm_glfw_error.c
  wm_glfw_input.c
  wm_glfw_window.c)

target_link_libraries(wmglfw glfw sl sys)
