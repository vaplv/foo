cmake_minimum_required(VERSION 2.6)

add_executable(app_main app_main.c)
target_link_libraries(app_main appcore game)

add_subdirectory(core)
add_subdirectory(game)

