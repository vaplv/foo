cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3")

file(GLOB RENDERER_FILES *.c)
add_library(renderer SHARED ${RENDERER_FILES})
target_link_libraries(renderer m mathssse rbi rbu sl sys)
set_target_properties(renderer PROPERTIES DEFINE_SYMBOL BUILD_RDR)
