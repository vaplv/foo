cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest/maths/)

add_executable(utest_maths utest_maths.c)
target_link_libraries(utest_maths mathssse)

add_test(utest_maths ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_maths)
