cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3")

add_library(mathssse SHARED 
  ../regular/aosf44.c
  ../regular/aosq.c
  sse.c)
