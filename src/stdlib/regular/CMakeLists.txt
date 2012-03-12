cmake_minimum_required(VERSION 2.6)

add_library(sl SHARED
  sl_hash_table.c
  sl_logger.c
  sl_set.c
  sl_string.c
  sl_wstring.c
  sl_vector.c)

target_link_libraries(sl sys)
