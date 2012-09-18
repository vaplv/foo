cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3")

add_definitions(
  -DRBI_USE_SHARED_LIBRARY 
  -DRBU_USE_SHARED_LIBRARY
  -DRDR_BUILD_SHARED_LIBRARY)

file(GLOB RENDERER_FILES *.c)
add_library(renderer SHARED ${RENDERER_FILES})

target_link_libraries(renderer m mathssse rbi rbu sl sys)
