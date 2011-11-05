cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest/app/core)

add_executable(utest_app utest_app.c)
target_link_libraries(utest_app appcore sys)

add_executable(utest_app_model utest_app_model.c)
target_link_libraries(utest_app_model appcore sys)

add_executable(utest_app_world utest_app_world.c)
target_link_libraries(utest_app_world appcore sys)

add_test(
  app_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_app
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  app_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_app
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

