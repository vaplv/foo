cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest/sys)

add_executable(utest_sys utest_sys)
target_link_libraries(utest_sys sl sys)

add_test(sys ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_sys)

