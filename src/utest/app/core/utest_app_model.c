#include "app/core/app.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
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
  struct app_args args = { NULL, NULL };
  struct app* app = NULL;
  struct app_model* model = NULL;
  struct app_model_instance* instance = NULL;
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

  CHECK(app_create_model(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_create_model(app, NULL, NULL), BAD_ARG);
  CHECK(app_create_model(NULL, NULL, &model), BAD_ARG);
  CHECK(app_create_model(app, NULL, &model), OK);

  CHECK(app_free_model(NULL, NULL), BAD_ARG);
  CHECK(app_free_model(app, NULL), BAD_ARG);
  CHECK(app_free_model(NULL, model), BAD_ARG);
  CHECK(app_free_model(app, model), OK);

  CHECK(app_create_model(app, PATH, &model), OK);

  CHECK(app_clear_model(NULL, NULL), BAD_ARG);
  CHECK(app_clear_model(app, NULL), BAD_ARG);
  CHECK(app_clear_model(NULL, model), BAD_ARG);
  CHECK(app_clear_model(app, model), OK);

  CHECK(app_load_model(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_load_model(app, NULL, NULL), BAD_ARG);
  CHECK(app_load_model(NULL, PATH, NULL), BAD_ARG);
  CHECK(app_load_model(app, PATH, NULL), BAD_ARG);
  CHECK(app_load_model(NULL, NULL, model), BAD_ARG);
  CHECK(app_load_model(app, NULL, model), BAD_ARG);
  CHECK(app_load_model(NULL, PATH, model), BAD_ARG);
  CHECK(app_load_model(app, PATH, model), OK);

  CHECK(app_instantiate_model(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_instantiate_model(app, NULL, NULL), BAD_ARG);
  CHECK(app_instantiate_model(NULL, model, NULL), BAD_ARG);
  CHECK(app_instantiate_model(app, model, NULL), BAD_ARG);
  CHECK(app_instantiate_model(NULL, NULL, &instance), BAD_ARG);
  CHECK(app_instantiate_model(app, NULL, &instance), BAD_ARG);
  CHECK(app_instantiate_model(NULL, model, &instance), BAD_ARG);
  CHECK(app_instantiate_model(app, model, &instance), OK);

  CHECK(app_free_model_instance(NULL, NULL), BAD_ARG);
  CHECK(app_free_model_instance(app, NULL), BAD_ARG);
  CHECK(app_free_model_instance(NULL, instance), BAD_ARG);
  CHECK(app_free_model_instance(app, instance), OK);

  CHECK(app_free_model(app, model), OK);

  CHECK(app_shutdown(app), OK);

  return 0;
}

