cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest/app/editor)

add_executable(utest_edit utest_edit.c)
target_link_libraries(utest_edit appcore editgtk)

configure_file(
  ${foo_SOURCE_DIR}/app/editor/gtk/edit_gtk.glade 
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/edit_gtk.glade 
  COPY_ONLY)

add_test(
  edit_null
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_edit
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbnull.so
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/edit_gtk.glade)

add_test(
  edit_ogl3
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utest_edit
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/librbogl3.so
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/edit_gtk.glade)
