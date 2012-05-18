cmake_minimum_required(VERSION 2.6)

file(GLOB EDIT_FILES *.c)
add_library(editor SHARED ${EDIT_FILES})

target_link_libraries(editor appcore)
