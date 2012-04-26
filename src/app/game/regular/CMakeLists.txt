cmake_minimum_required(VERSION 2.6)

file(GLOB GAME_FILES *.c)
add_library(game SHARED ${GAME_FILES})
