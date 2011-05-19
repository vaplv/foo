cmake_minimum_required(VERSION 2.8)
project(render_system C)


set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${render_system_SOURCE_DIR}/../lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${render_system_SOURCE_DIR}/../bin)
set(CMAKE_VERBOSE_MAKEFILE false)

set(CMAKE_BUILD_TYPE "DEBUG")
#set(CMAKE_C_FLAGS "-Wall -pedantic -std=c99 -Wcast-align -ffast-math")
set(CMAKE_C_FLAGS "")
#set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
 
include_directories(. /usr/include /usr/local/include)
link_directories(/usr/lib /usr/local/lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

add_subdirectory(window_manager)
add_subdirectory(render_backend)
add_subdirectory(examples)
