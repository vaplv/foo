cmake_minimum_required(VERSION 2.6)

find_path(GL_INC_PATH NAMES GL/gl.h /GL/glext)
find_library(GL_LIB_PATH NAMES GL)

if(NOT GL_INC_PATH)
  message(FATAL_ERROR "Can't find the OpengGL headers.")
endif()

if(NOT GL_LIB_PATH)
  message(FATAL_ERROR "Can't find the OpenGL libraries.")
endif()

file(GLOB RBOGL3_FILES *.c)

add_definitions(-DGL_GLEXT_PROTOTYPES)
add_library(rbogl3 SHARED ${RBOGL3_FILES})

target_link_libraries(rbogl3 GL GLU)

