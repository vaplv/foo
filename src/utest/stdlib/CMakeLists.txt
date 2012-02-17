cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest/stdlib)

add_executable(utest_sl_hash_table utest_sl_hash_table.c)
target_link_libraries(utest_sl_hash_table sl)

add_executable(utest_sl_logger utest_sl_logger.c)
target_link_libraries(utest_sl_logger sl)

add_executable(utest_sl_set utest_sl_set.c)
target_link_libraries(utest_sl_set sl)

add_executable(utest_sl_string utest_sl_string.c)
target_link_libraries(utest_sl_string sl)

add_executable(utest_sl_vector utest_sl_vector.c)
target_link_libraries(utest_sl_vector sl)

add_test(
  sl_hash_table
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_sl_hash_table)

add_test(
  sl_logger
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_sl_logger)

add_test(
  sl_vector
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_sl_vector)

add_test(
  sl_set
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_sl_set)

add_test(
  sl_string
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_sl_string)

