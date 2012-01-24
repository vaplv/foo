#include "app/core/app.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "sys/mem_allocator.h"
#include "utest/app/core/cube_obj.h"
#include "utest/utest.h"
#include "sys/sys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH "/tmp/cube.obj"

#define OK APP_NO_ERROR
#define BAD_ARG APP_INVALID_ARGUMENT

static void
cbk(struct app_model* model, void* data)
{
  struct app_model** m = data;
  (*m) = model;
}

int
main(int argc, char** argv)
{
  struct app_args args = { NULL, NULL, NULL };
  struct app* app = NULL;
  struct app_model* model = NULL;
  struct app_model* model2 = NULL;
  struct app_model* model3 = NULL;
  struct app_model* model_list[5];
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

  CHECK(app_attach_callback
    (NULL, APP_SIGNAL_CREATE_MODEL, NULL, NULL), BAD_ARG);
  CHECK(app_attach_callback
    (app, APP_SIGNAL_CREATE_MODEL, NULL, NULL), BAD_ARG);
  CHECK(app_attach_callback
    (NULL, APP_SIGNAL_CREATE_MODEL, APP_CALLBACK(cbk), NULL), BAD_ARG);
  CHECK(app_attach_callback
    (app, APP_SIGNAL_CREATE_MODEL, APP_CALLBACK(cbk), NULL), OK);

  CHECK(app_detach_callback
    (NULL, APP_SIGNAL_DESTROY_MODEL, NULL, NULL), BAD_ARG);
  CHECK(app_detach_callback
    (app, APP_SIGNAL_DESTROY_MODEL, NULL, NULL), BAD_ARG);
  CHECK(app_detach_callback
    (NULL, APP_SIGNAL_CREATE_MODEL, NULL, NULL), BAD_ARG);
  CHECK(app_detach_callback
    (app, APP_SIGNAL_CREATE_MODEL, NULL, NULL), BAD_ARG);
  CHECK(app_detach_callback
    (NULL, APP_SIGNAL_DESTROY_MODEL, APP_CALLBACK(cbk), NULL), BAD_ARG);
  CHECK(app_detach_callback
    (app, APP_SIGNAL_DESTROY_MODEL, APP_CALLBACK(cbk), NULL), BAD_ARG);
  CHECK(app_detach_callback
    (NULL, APP_SIGNAL_CREATE_MODEL, APP_CALLBACK(cbk), NULL), BAD_ARG);
  CHECK(app_detach_callback
    (app, APP_SIGNAL_CREATE_MODEL, APP_CALLBACK(cbk), NULL), OK);

  CHECK(app_attach_callback
    (app, APP_SIGNAL_CREATE_MODEL, APP_CALLBACK(cbk), &model2), OK);
  CHECK(app_attach_callback
    (app, APP_SIGNAL_DESTROY_MODEL, APP_CALLBACK(cbk), &model3), OK);

  model2 = NULL;
  CHECK(app_create_model(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_create_model(app, NULL, NULL), BAD_ARG);
  CHECK(app_create_model(NULL, NULL, &model), BAD_ARG);
  CHECK(app_create_model(app, NULL, &model), OK);
  CHECK(model, model2);

  model3 = NULL;
  CHECK(app_model_ref_put(NULL), BAD_ARG);
  CHECK(app_model_ref_put(model), OK);
  CHECK(model, model3);

  model2 = NULL;
  CHECK(app_create_model(app, PATH, &model), OK);
  CHECK(model, model2);

  model2 = NULL;
  CHECK(app_clear_model(NULL), BAD_ARG);
  CHECK(app_clear_model(model), OK);
  CHECK(model2, NULL);

  CHECK(app_load_model(NULL, NULL), BAD_ARG);
  CHECK(app_load_model(PATH, NULL), BAD_ARG);
  CHECK(app_load_model(NULL, model), BAD_ARG);
  CHECK(app_load_model(PATH, model), OK);
  CHECK(model2, NULL);

  /* This model may be freed when the application will be shut down. */
  for(i = 0; i < sizeof(model_list) / sizeof(struct model*); ++i)
    CHECK(app_create_model(app, PATH, &model_list[i]), OK);

  CHECK(app_instantiate_model(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_instantiate_model(app, NULL, NULL), BAD_ARG);
  CHECK(app_instantiate_model(NULL, model, NULL), BAD_ARG);
  CHECK(app_instantiate_model(app, model, NULL), BAD_ARG);
  CHECK(app_instantiate_model(NULL, NULL, &instance), BAD_ARG);
  CHECK(app_instantiate_model(app, NULL, &instance), BAD_ARG);
  CHECK(app_instantiate_model(NULL, model, &instance), BAD_ARG);
  CHECK(app_instantiate_model(app, model, &instance), OK);

  CHECK(app_model_instance_ref_get(NULL), BAD_ARG);
  CHECK(app_model_instance_ref_get(instance), OK);
  CHECK(app_model_instance_ref_put(NULL), BAD_ARG);
  CHECK(app_model_instance_ref_put(instance), OK);
  CHECK(app_model_instance_ref_put(instance), OK);

  CHECK(app_model_ref_get(NULL), BAD_ARG);
  CHECK(app_model_ref_get(model), OK);
  CHECK(app_model_ref_put(model), OK);
  CHECK(app_model_ref_put(model), OK);

  CHECK(app_ref_put(app), OK);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;
}

