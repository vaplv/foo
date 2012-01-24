cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig REQUIRED)

pkg_check_modules(FREETYPE REQUIRED freetype2)
include_directories(${FREETYPE_INCLUDE_DIRS})
link_directories(${FREETYPE_LIBRARY_DIRS})

add_library(rsrc SHARED
  rsrc.c
  rsrc_context.c
  rsrc_error.c
  rsrc_font.c
  rsrc_geometry.c
  rsrc_wavefront_obj.c)

target_link_libraries(rsrc sl sys ${FREETYPE_LIBRARIES})
