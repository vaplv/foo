cmake_minimum_required(VERSION 2.6)

find_path(GLFW_INC_PATH argtable2.h)
find_library(GLFW_LIB_PATH argtable2)

if(NOT GLFW_INC_PATH)
  message(FATAL_ERROR "Can't find GL/glfw.h.")
endif()

if(NOT GLFW_LIB_PATH)
  message(FATAL_ERROR "Can't find glfw.")
endif()

add_definitions(
   -DRDR_USE_SHARED_LIBRARY
   -DRSRC_USE_SHARED_LIBRARY
   -DAPP_BUILD_SHARED_LIBRARY)

file(GLOB APPCORE_FILES *.c)
add_library(appcore SHARED ${APPCORE_FILES})

target_link_libraries(appcore renderer rsrc sl sys wmglfw argtable2)
