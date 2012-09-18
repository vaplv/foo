cmake_minimum_required(VERSION 2.6)

add_definitions(-DRBI_BUILD_SHARED_LIBRARY)

file(GLOB RBI_FILES *.c)
add_library(rbi SHARED ${RBI_FILES})
target_link_libraries(rbi dl)

