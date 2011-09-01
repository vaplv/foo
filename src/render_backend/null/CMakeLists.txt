cmake_minimum_required(VERSION 2.6)

add_library(rbnull SHARED rb_null.c)
target_link_libraries(rbnull)
