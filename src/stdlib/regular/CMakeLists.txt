cmake_minimum_required(VERSION 2.6)

add_definitions(-DSL_BUILD_SHARED_LIBRARY)

file(GLOB STDLIB_FILES *.c)
add_library(sl SHARED ${STDLIB_FILES})
target_link_libraries(sl sys)
set_target_properties(sl PROPERTIES DEFINE_SYMBOL BUILD_SL)

