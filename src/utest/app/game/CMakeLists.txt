cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest/app/game)

add_executable(utest_game utest_game.c)
target_link_libraries(utest_game game appcore sys)

add_test(
  game_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_game
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  game_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_game
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)


