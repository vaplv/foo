cmake_minimum_required(VERSION 2.6)

file(GLOB RBI_FILES *.c)
add_library(rbi SHARED ${RBI_FILES})
target_link_libraries(rbi dl)
set_target_properties(rbi PROPERTIES DEFINE_SYMBOL BUILD_RBI)

