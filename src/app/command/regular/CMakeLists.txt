cmake_minimum_required(VERSION 2.6)

file(GLOB COMMAND_FILES *.c)
add_library(command SHARED ${COMMAND_FILES})
