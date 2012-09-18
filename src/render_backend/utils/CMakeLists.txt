cmake_minimum_required(VERSION 2.6)

add_definitions(-DRBU_BUILD_SHARED_LIBRARY)

file(GLOB RBU_FILES *.c)
add_library(rbu SHARED ${RBU_FILES})

