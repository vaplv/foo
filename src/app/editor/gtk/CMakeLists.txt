cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)

include_directories(${GTK_INCLUDE_DIRS})
link_directories(${GTK_LIBRARY_DIRS})

add_library(editgtk SHARED 
  edit_gtk.c
  edit_gtk_error.c
  edit_gtk_explorer.c)

target_link_libraries(editgtk sys ${GTK_LIBRARIES})

