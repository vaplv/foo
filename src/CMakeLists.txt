cmake_minimum_required(VERSION 2.6)
project(foo C)
enable_testing()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${foo_SOURCE_DIR}/../lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${foo_SOURCE_DIR}/../bin)

if(NOT CMAKE_COMPILER_IS_GNUCC)
  message(FATAL_ERROR "Unsupported compiler.")
endif(NOT CMAKE_COMPILER_IS_GNUCC)

set(CMAKE_C_FLAGS 
  "-pedantic -std=c99 -Wall -Wextra -Wcast-align -Wmissing-declarations -Wmissing-prototypes -ffast-math -fvisibility=hidden -msse3 -Wl,-z,defs")
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

if(UNIX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_POSIX_C_SOURCE=200809L")
else(UNIX)
  message(FATAL_ERROR "Unsupported platform.")
endif(UNIX)

find_program(CPPCHECK cppcheck)
if(CPPCHECK) 
  add_custom_target(static_check 
    cppcheck ${foo_SOURCE_DIR} --quiet --enable=all -I${foo_SOURCE_DIR})
else(CPPCHECK)
  message(STATUS "cppcheck is not found: disable static_check target")
endif(CPPCHECK)

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

