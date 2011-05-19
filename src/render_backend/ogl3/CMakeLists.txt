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
add_library(rbogl3 SHARED 
  rb_ogl3_buffers.c
  rb_ogl3_context.c
  rb_ogl3_miscellaneous.c
  rb_ogl3_program.c
  rb_ogl3_shader.c
  rb_ogl3_texture.c
  rb_ogl3_uniform.c
  rb_ogl3_vertex_array.c)
target_link_libraries(rbogl3 GL GLU)
