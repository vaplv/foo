cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3")

add_library(renderer SHARED
  rdr_attrib.c
  rdr_error.c
  rdr_font.c
  rdr_material.c
  rdr_mesh.c
  rdr_model.c
  rdr_model_instance.c
  rdr_system.c
  rdr_term.c
  rdr_world.c)

target_link_libraries(renderer m mathssse rbi sl sys)
