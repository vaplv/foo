#include "app/core/app.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "app/core/app_world.h"
#include "utest/app/core/cube_obj.h"
#include "utest/utest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH "/tmp/cube.obj"

#define OK APP_NO_ERROR
#define BAD_ARG APP_INVALID_ARGUMENT

int
main(int argc, char** argv)
{
  struct app_args args = { NULL, NULL, NULL };
  struct app* app = NULL;
  struct app_model* model = NULL;
  struct app_model_instance* instances[2] = { NULL, NULL };
  struct app_world* world = NULL;
  FILE* fp = NULL;
  size_t i = 0;

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    return -1;
  }
  args.render_driver = argv[1];

  /* Create the temporary obj file. */
  fp = fopen(PATH, "w");
  NCHECK(fp, NULL);
  i = fwrite(cube_obj, sizeof(char), strlen(cube_obj), fp);
  CHECK(strlen(cube_obj) * sizeof(char), i);
  CHECK(fclose(fp), 0);

  CHECK(app_init(&args, &app), OK);
  CHECK(app_create_model(app, PATH, &model), OK);
  CHECK(app_instantiate_model(app, model, &instances[0]), OK);
  CHECK(app_instantiate_model(app, model, &instances[1]), OK);

  CHECK(app_create_world(NULL, NULL), BAD_ARG);
  CHECK(app_create_world(app, NULL), BAD_ARG);
  CHECK(app_create_world(NULL, &world), BAD_ARG);
  CHECK(app_create_world(app, &world), OK);

  CHECK(app_world_add_model_instances(NULL, NULL, 0, NULL), BAD_ARG);
  CHECK(app_world_add_model_instances(app, NULL, 0, NULL), BAD_ARG);
  CHECK(app_world_add_model_instances(NULL, world, 0, NULL), BAD_ARG);
  CHECK(app_world_add_model_instances(app, world, 0, NULL), OK);
  CHECK(app_world_add_model_instances(NULL, NULL, 0, instances), BAD_ARG);
  CHECK(app_world_add_model_instances(app, NULL, 0, instances), BAD_ARG);
  CHECK(app_world_add_model_instances(NULL, world, 0, instances), BAD_ARG);
  CHECK(app_world_add_model_instances(app, world, 0, instances), OK);
  CHECK(app_world_add_model_instances(NULL, NULL, 2, NULL), BAD_ARG);
  CHECK(app_world_add_model_instances(app, NULL, 2, NULL), BAD_ARG);
  CHECK(app_world_add_model_instances(NULL, world, 2, NULL), BAD_ARG);
  CHECK(app_world_add_model_instances(app, world, 2, NULL), BAD_ARG);
  CHECK(app_world_add_model_instances(NULL, NULL, 2, instances), BAD_ARG);
  CHECK(app_world_add_model_instances(app, NULL, 2, instances), BAD_ARG);
  CHECK(app_world_add_model_instances(NULL, world, 2, instances), BAD_ARG);
  CHECK(app_world_add_model_instances(app, world, 2, instances), OK);

  CHECK(app_world_remove_model_instances(NULL, NULL, 0, NULL), BAD_ARG);
  CHECK(app_world_remove_model_instances(app, NULL, 0, NULL), BAD_ARG);
  CHECK(app_world_remove_model_instances(NULL, world, 0, NULL), BAD_ARG);
  CHECK(app_world_remove_model_instances(app, world, 0, NULL), OK);
  CHECK(app_world_remove_model_instances(NULL, NULL, 1, NULL), BAD_ARG);
  CHECK(app_world_remove_model_instances(app, NULL, 1, NULL), BAD_ARG);
  CHECK(app_world_remove_model_instances(NULL, world, 1, NULL), BAD_ARG);
  CHECK(app_world_remove_model_instances(app, world, 1, NULL), BAD_ARG);
  CHECK(app_world_remove_model_instances(NULL, NULL, 0, &instances[0]), BAD_ARG);
  CHECK(app_world_remove_model_instances(app, NULL, 0, &instances[0]), BAD_ARG);
  CHECK(app_world_remove_model_instances(NULL, world, 0, &instances[0]), BAD_ARG);
  CHECK(app_world_remove_model_instances(app, world, 0, &instances[0]), OK);
  CHECK(app_world_remove_model_instances(NULL, NULL, 1, &instances[0]), BAD_ARG);
  CHECK(app_world_remove_model_instances(app, NULL, 1, &instances[0]), BAD_ARG);
  CHECK(app_world_remove_model_instances(NULL, world, 1, &instances[0]), BAD_ARG);
  CHECK(app_world_remove_model_instances(app, world, 1, &instances[0]), OK);
  CHECK(app_world_remove_model_instances(app, world, 1, &instances[1]), OK);
  CHECK(app_world_remove_model_instances(app, world, 1, &instances[0]), BAD_ARG);

  CHECK(app_free_world(NULL, NULL), BAD_ARG);
  CHECK(app_free_world(app, NULL), BAD_ARG);
  CHECK(app_free_world(NULL, world), BAD_ARG);
  CHECK(app_free_world(app, world), OK);

  CHECK(app_free_model_instance(app, instances[1]), OK);
  CHECK(app_free_model_instance(app, instances[0]), OK);
  CHECK(app_free_model(app, model), OK);
  CHECK(app_shutdown(app), OK);

  return 0;
}

