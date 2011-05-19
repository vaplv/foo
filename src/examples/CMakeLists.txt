cmake_minimum_required(VERSION 2.6)

add_executable(eg_render_backend eg_render_backend.c)
add_definitions(-DMALLOC_FUNC_CHECK)
target_link_libraries(eg_render_backend wmglfw rbogl3)
