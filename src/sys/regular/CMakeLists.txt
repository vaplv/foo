cmake_minimum_required(VERSION 2.6)

file(GLOB SYS_FILES *.c)
add_library(sys SHARED ${SYS_FILES})
