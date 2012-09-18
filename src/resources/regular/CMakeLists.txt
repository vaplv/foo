cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig REQUIRED)

pkg_check_modules(FREETYPE REQUIRED freetype2)
include_directories(${FREETYPE_INCLUDE_DIRS})
link_directories(${FREETYPE_LIBRARY_DIRS})

add_definitions(-DRSRC_BUILD_SHARED_LIBRARY)

file(GLOB RSRC_FILES *.c)
add_library(rsrc SHARED ${RSRC_FILES})

target_link_libraries(rsrc sl sys ${FREETYPE_LIBRARIES})
