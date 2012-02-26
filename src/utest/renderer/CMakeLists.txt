cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest/renderer)

add_executable(utest_rdr_font utest_rdr_font.c)
target_link_libraries(utest_rdr_font wmglfw renderer rsrc)

add_executable(utest_rdr_frame utest_rdr_frame.c)
target_link_libraries(utest_rdr_frame wmglfw renderer)

add_executable(utest_rdr_world utest_rdr_world.c)
target_link_libraries(utest_rdr_world wmglfw renderer)

add_executable(utest_renderer utest_renderer.c)
target_link_libraries(utest_renderer wmglfw renderer)

add_executable(utest_rdr_term utest_rdr_term.c)
target_link_libraries(utest_rdr_term wmglfw renderer)

add_test(
  renderer_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_renderer
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  renderer_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_renderer
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)

add_test(
  rdr_font_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_rdr_font
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../etc/fonts/freefont-ttf/FreeSans.ttf)

add_test(
  rdr_font_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_rdr_font
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../etc/fonts/freefont-ttf/FreeSans.ttf)

add_test(
  rdr_frame_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_rdr_frame
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  rdr_frame_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_rdr_frame
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)

add_test(
  rdr_term_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_rdr_term
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  rdr_term_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_rdr_term
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)

add_test(
  rdr_world_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_rdr_world
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  rdr_world_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_rdr_world
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)

