cmake_minimum_required(VERSION 2.6)

file(GLOB RBNULL_FILES *.c)
add_library(rbnull SHARED ${RBNULL_FILES})
target_link_libraries(rbnull)

