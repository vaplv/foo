cmake_minimum_required(VERSION 2.6)

find_path(GLFW_INC_PATH GL/glfw.h)
find_library(GLFW_LIB_PATH glfw)

if(NOT GLFW_INC_PATH)
  message(FATAL_ERROR "Can't find GL/glfw.h.")
endif()

if(NOT GLFW_LIB_PATH)
  message(FATAL_ERROR "Can't find glfw.")
endif()

add_definitions(-DWM_BUILD_SHARED_LIBRARY)

file(GLOB WMGLFW_FILES *.c)
add_library(wmglfw SHARED ${WMGLFW_FILES})

target_link_libraries(wmglfw glfw sl sys)
set_target_properties(wmglfw PROPERTIES DEFINE_SYMBOL BUILD_WM)
