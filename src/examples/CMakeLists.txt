cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/examples)

add_executable(eg_render_backend eg_render_backend.c)
target_link_libraries(eg_render_backend wmglfw rbi m)
set_target_properties(
  eg_render_backend PROPERTIES 
  COMPILE_FLAGS "-DWM_USE_SHARED_LIBRARY -DRBI_USE_SHARED_LIBRARY")

add_executable(eg_rdr_model eg_rdr_model.c)
target_link_libraries(eg_rdr_model wmglfw renderer)
set_target_properties(
  eg_rdr_model PROPERTIES
  COMPILE_FLAGS "-DWM_USE_SHARED_LIBRARY -DRDR_USE_SHARED_LIBRARY")

add_executable(eg_rdr_term eg_rdr_term.c)
target_link_libraries(eg_rdr_term wmglfw renderer rsrc)
set_target_properties(
  eg_rdr_term PROPERTIES
  COMPILE_FLAGS 
  "-DWM_USE_SHARED_LIBRARY -DRDR_USE_SHARED_LIBRARY -DRSRC_USE_SHARED_LIBRARY")

add_executable(eg_rsrc_wavefront_obj eg_rsrc_wavefront_obj.c)
target_link_libraries(eg_rsrc_wavefront_obj rsrc sys)
set_target_properties(
  eg_rsrc_wavefront_obj PROPERTIES
  COMPILE_FLAGS "-DRSRC_USE_SHARED_LIBRARY -DSYS_USE_SHARED_LIBRARY")
