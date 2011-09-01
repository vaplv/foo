cmake_minimum_required(VERSION 2.6)

add_executable(utest_renderer utest_renderer.c)
target_link_libraries(utest_renderer wmglfw renderer)

add_executable(utest_rdr_world utest_rdr_world.c)
target_link_libraries(utest_rdr_world wmglfw renderer)

add_executable(utest_sl_linked_list utest_sl_linked_list.c)
target_link_libraries(utest_sl_linked_list sl)

add_executable(utest_sl_sorted_vector utest_sl_sorted_vector.c)
target_link_libraries(utest_sl_sorted_vector sl)

add_executable(utest_sl_vector utest_sl_vector.c)
target_link_libraries(utest_sl_vector sl)

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

add_test(
  sl_linked_list
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_sl_linked_list)

add_test(
  sl_vector
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_sl_vector)

add_test(
  sl_sorted_vector
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_sl_sorted_vector)

