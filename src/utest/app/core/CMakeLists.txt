cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest/app/core)

add_executable(utest_app utest_app.c)
target_link_libraries(utest_app appcore)

add_executable(utest_app_command utest_app_command.c)
target_link_libraries(utest_app_command appcore)

add_executable(utest_app_command_buffer utest_app_command_buffer.c)
target_link_libraries(utest_app_command_buffer appcore)

add_executable(utest_app_model utest_app_model.c)
target_link_libraries(utest_app_model appcore)

add_executable(utest_app_world utest_app_world.c)
target_link_libraries(utest_app_world appcore)

add_test(
  app_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_app
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  app_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_app
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)

add_test(
  app_command_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_app_command
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  app_command_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_app_command
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)

add_test(
  app_command_buffer_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_app_command_buffer
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  app_command_buffer_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_app_command_buffer
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)

add_test(
  app_model_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_app_model
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  app_model_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_app_model
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)

add_test(
  app_world_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_app_world
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  app_world_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_app_world
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)

