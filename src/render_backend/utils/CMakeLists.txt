cmake_minimum_required(VERSION 2.6)

file(GLOB RBU_FILES *.c)
add_library(rbu SHARED ${RBU_FILES})
target_link_libraries(rbu m)
set_target_properties(rbu PROPERTIES DEFINE_SYMBOL BUILD_RBU)

