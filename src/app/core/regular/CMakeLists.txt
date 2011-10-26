cmake_minimum_required(VERSION 2.6)

add_library(appcore SHARED 
  app.c
  app_error.c
  app_model.c
  app_model_instance.c
  app_view.c
  app_world.c)

target_link_libraries(appcore renderer rsrc sl wmglfw)
