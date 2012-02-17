cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest/sys/)

add_executable(utest_list utest_list.c)
target_link_libraries(utest_list sys)

add_executable(utest_mem_allocator utest_mem_allocator.c)
target_link_libraries(utest_mem_allocator sys)

add_test(list ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_list)
add_test(mem_allocator ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_mem_allocator)
