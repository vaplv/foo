cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3")


file(GLOB MATHS_FILES *.c ../regular/*.c)
add_library(mathssse SHARED ${MATHS_FILES})
set_target_properties(mathssse PROPERTIES DEFINE_SYMBOL BUILD_SIMD)

