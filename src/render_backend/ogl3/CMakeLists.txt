cmake_minimum_required(VERSION 2.6)

find_path(GL_INC_PATH NAMES GL/gl.h /GL/glext)
find_library(GL_LIB_PATH NAMES GL)

if(NOT GL_INC_PATH)
  message(FATAL_ERROR "Can't find the OpengGL headers.")
endif()

if(NOT GL_LIB_PATH)
  message(FATAL_ERROR "Can't find the OpenGL libraries.")
endif()

add_definitions(-DGL_GLEXT_PROTOTYPES)

file(GLOB RBOGL3_FILES *.c)
add_library(rbogl3 SHARED ${RBOGL3_FILES})
target_link_libraries(rbogl3 sys)
set_target_properties(rbogl3 PROPERTIES DEFINE_SYMBOL BUILD_RB)

target_link_libraries(rbogl3 GL GLU)

