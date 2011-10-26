cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest/renderer)

add_executable(utest_renderer utest_renderer.c)
target_link_libraries(utest_renderer wmglfw renderer)

add_executable(utest_rdr_world utest_rdr_world.c)
target_link_libraries(utest_rdr_world wmglfw renderer)

add_test(
  renderer_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_renderer
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  renderer_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_renderer
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)

add_test(
  rdr_world_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_rdr_world
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  rdr_world_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_rdr_world
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)

