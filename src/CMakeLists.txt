cmake_minimum_required(VERSION 2.6)
project(foo C)
enable_testing()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${foo_SOURCE_DIR}/../lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${foo_SOURCE_DIR}/../bin)
set(CMAKE_VERBOSE_MAKEFILE false)

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_C_FLAGS 
  "-pedantic -std=c99 -Wall -Wcast-align -Wextra -Wmissing-declarations -Wmissing-prototypes -ffast-math -fvisibility=hidden -msse3")
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
 
include_directories(. /usr/include /usr/local/include)
link_directories(/usr/lib /usr/local/lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

add_subdirectory(app)
add_subdirectory(examples)
add_subdirectory(maths)
add_subdirectory(render_backend)
add_subdirectory(renderer)
add_subdirectory(resources)
add_subdirectory(stdlib)
add_subdirectory(sys)
add_subdirectory(utest)
add_subdirectory(window_manager)

