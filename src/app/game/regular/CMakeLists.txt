cmake_minimum_required(VERSION 2.6)

file(GLOB GAME_FILES *.c)
add_library(game SHARED ${GAME_FILES})
set_target_properties(game PROPERTIES DEFINE_SYMBOL BUILD_GAME)
