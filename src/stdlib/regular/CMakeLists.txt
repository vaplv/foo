cmake_minimum_required(VERSION 2.6)

add_library(sl SHARED
  sl_context.c
  sl_linked_list.c
  sl_sorted_vector.c
  sl_vector.c)
