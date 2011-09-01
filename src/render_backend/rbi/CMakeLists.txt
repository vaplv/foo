cmake_minimum_required(VERSION 2.6)

add_library(rbi SHARED rbi.c)
target_link_libraries(rbi dl)

