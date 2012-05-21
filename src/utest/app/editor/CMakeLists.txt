cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest/app/editor)

add_executable(utest_editor utest_editor.c)
target_link_libraries(utest_editor editor appcore)

add_executable(
  utest_edit_model_instance_selection 
  utest_edit_model_instance_selection.c)
target_link_libraries(utest_edit_model_instance_selection  editor appcore)

add_test(
  editor_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_editor
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  editor_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_editor
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)

add_test(
  edit_model_instance_selection_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_edit_model_instance_selection
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so)

add_test(
  edit_model_instance_selection_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_edit_model_instance_selection
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so)

