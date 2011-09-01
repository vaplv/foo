cmake_minimum_required(VERSION 2.6)

add_executable(eg_render_backend eg_render_backend.c)
target_link_libraries(eg_render_backend wmglfw rbi m)

add_executable(eg_rdr_model eg_rdr_model.c)
target_link_libraries(eg_rdr_model wmglfw renderer)
