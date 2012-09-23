cmake_minimum_required(VERSION 2.6)

add_definitions(-DRB_BUILD_SHARED_LIBRARY)

file(GLOB RBNULL_FILES *.c)
add_library(rbnull SHARED ${RBNULL_FILES})
target_link_libraries(rbnull)
set_target_properties(rbnull PROPERTIES DEFINE_SYMBOL BUILD_RB)

