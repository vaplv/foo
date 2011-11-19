cmake_minimum_required(VERSION 2.6)

add_library(rsrc SHARED
  rsrc_error.c
  rsrc_context.c
  rsrc_geometry.c
  rsrc_wavefront_obj.c)

target_link_libraries(rsrc sl sys)
