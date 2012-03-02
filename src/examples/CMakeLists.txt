cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/examples)

add_executable(eg_render_backend eg_render_backend.c)
target_link_libraries(eg_render_backend wmglfw rbi m)

add_executable(eg_rdr_model eg_rdr_model.c)
target_link_libraries(eg_rdr_model wmglfw renderer)

add_executable(eg_rdr_term eg_rdr_term.c)
target_link_libraries(eg_rdr_term wmglfw renderer rsrc)

add_executable(eg_rsrc_wavefront_obj eg_rsrc_wavefront_obj.c)
target_link_libraries(eg_rsrc_wavefront_obj rsrc)
