cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest/resources)

add_executable(utest_rsrc_font utest_rsrc_font.c)
target_link_libraries(utest_rsrc_font rsrc)

add_test(
  rsrc_font
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_rsrc_font
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../etc/fonts/freefont-ttf/FreeSans.ttf)

