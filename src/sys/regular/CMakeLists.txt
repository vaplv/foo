cmake_minimum_required(VERSION 2.6)

add_definitions(-DSYS_BUILD_SHARED_LIBRARY)

file(GLOB SYS_FILES *.c)
add_library(sys SHARED ${SYS_FILES})
target_link_libraries(sys rt)
set_target_properties(sys PROPERTIES DEFINE_SYMBOL BUILD_SYS)
