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
  #define MODEL_COUNT 5
  struct app_args args = { NULL, NULL, NULL, NULL };
  struct app* app = NULL;
  struct app_model* model = NULL;
  struct app_model* model2 = NULL;
  struct app_model* model3 = NULL;
  struct app_model* model_list[MODEL_COUNT];
  struct app_model_instance* instance = NULL;
  struct app_model_instance* instance1 = NULL;
  const char** lst = NULL;
  const char* cstr = NULL;
  FILE* fp = NULL;
  size_t i = 0;
  size_t len = 0;

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
  CHECK(app_create_model(NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_create_model(app, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_create_model(NULL, NULL, NULL, &model), BAD_ARG);
  CHECK(app_create_model(app, NULL, NULL, &model), OK);
  CHECK(model, model2);

  model3 = NULL;
  CHECK(app_model_ref_put(NULL), BAD_ARG);
  CHECK(app_model_ref_put(model), OK);
  CHECK(model, model3);

  model2 = NULL;
  CHECK(app_create_model(app, PATH, NULL, &model), OK);
  CHECK(model, model2);

  model2 = NULL;
  CHECK(app_clear_model(NULL), BAD_ARG);
  CHECK(app_clear_model(model), OK);
  CHECK(model2, NULL);

  CHECK(app_load_model(NULL, NULL), BAD_ARG);
  CHECK(app_load_model(PATH, NULL), BAD_ARG);
  CHECK(app_load_model(NULL, model), BAD_ARG);
  CHECK(app_load_model(PATH, model), OK);

  CHECK(app_model_path(NULL, NULL), BAD_ARG);
  CHECK(app_model_path(model, NULL), BAD_ARG);
  CHECK(app_model_path(NULL, &cstr), BAD_ARG);
  CHECK(app_model_path(model, &cstr), OK);
  CHECK(strcmp(cstr, PATH), 0);

  CHECK(app_set_model_name(NULL, NULL), BAD_ARG);
  CHECK(app_set_model_name(model, NULL), BAD_ARG);
  CHECK(app_set_model_name(NULL, "Mdl0"), BAD_ARG);
  CHECK(app_set_model_name(model, "Mdl0"), OK);
  CHECK(app_set_model_name(model, "Mdl0"), OK); /* Set the same name. */
  CHECK(model2, NULL);

  CHECK(app_model_name(NULL, NULL), BAD_ARG);
  CHECK(app_model_name(model, NULL), BAD_ARG);
  CHECK(app_model_name(NULL, &cstr), BAD_ARG);
  CHECK(app_model_name(model, &cstr), OK);
  CHECK(strcmp(cstr, "Mdl0"), 0);

  CHECK(app_get_model(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_get_model(app, NULL, NULL), BAD_ARG);
  CHECK(app_get_model(NULL, "Mdl0", NULL), BAD_ARG);
  CHECK(app_get_model(app, "Mdl0", NULL), BAD_ARG);
  CHECK(app_get_model(NULL, NULL, &model), BAD_ARG);
  CHECK(app_get_model(app, NULL, &model), BAD_ARG);
  CHECK(app_get_model(NULL, "Mdl0", &model), BAD_ARG);
  CHECK(app_get_model(app, "Mdl0", &model), OK);
  CHECK(app_model_name(model, &cstr), OK);
  CHECK(strcmp(cstr, "Mdl0"), 0);

  /* This model may be freed when the application will be shut down. */
  for(i = 0; i < MODEL_COUNT; ++i) {
    char name[8];
    STATIC_ASSERT(MODEL_COUNT < 9999, Unexpected_MODEL_COUNT);
    snprintf(name, 7, "mdl%zu", i + 1);
    name[7] = '\0';
    CHECK(app_create_model(app, PATH, name, &model_list[i]), OK);
    CHECK(app_model_name(model_list[i], &cstr), OK);
    CHECK(strcmp(cstr, name), 0);
  }

  CHECK(app_model_name_completion(NULL, NULL, 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_name_completion(app, NULL, 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_name_completion(NULL, "M", 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_name_completion(app, "M", 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_name_completion(NULL, NULL, 0, &len, NULL), BAD_ARG);
  CHECK(app_model_name_completion(app, NULL, 0, &len, NULL), BAD_ARG);
  CHECK(app_model_name_completion(NULL, "M", 0, &len, NULL), BAD_ARG);
  CHECK(app_model_name_completion(app, "M", 0, &len, NULL), BAD_ARG);
  CHECK(app_model_name_completion(NULL, NULL, 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_name_completion(app, NULL, 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_name_completion(NULL, "M", 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_name_completion(app, "M", 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_name_completion(NULL, NULL, 0, &len, &lst), BAD_ARG);
  CHECK(app_model_name_completion(app, NULL, 1, &len, &lst), BAD_ARG);
  CHECK(app_model_name_completion(app, NULL, 0, &len, &lst), OK);
  CHECK(len, MODEL_COUNT + 1);
  NCHECK(lst, NULL);
  for(i = 1; i < MODEL_COUNT + 1; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
  }
  CHECK(app_model_name_completion(NULL, "M", 0, &len, &lst), BAD_ARG);
  CHECK(app_model_name_completion(app, "M", 0, &len, &lst), OK);
  CHECK(len, MODEL_COUNT + 1);
  NCHECK(lst, NULL);
  for(i = 1; i < MODEL_COUNT + 1; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
  }
  CHECK(app_model_name_completion(app, "Mxx", 1, &len, &lst), OK);
  CHECK(len, 1);
  NCHECK(lst, NULL);
  CHECK(strcmp(lst[0], "Mdl0"), 0);
  CHECK(app_model_name_completion(app, "Mxx", 2, &len, &lst), OK);
  CHECK(len, 0);
  CHECK(lst, NULL);
  CHECK(app_model_name_completion(app, "mdl", 3, &len, &lst), OK);
  CHECK(len, MODEL_COUNT);
  NCHECK(lst, NULL);
  CHECK(strncmp(lst[0], "mdl", 3), 0);
  for(i = 1; i < MODEL_COUNT; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
    CHECK(strncmp(lst[i], "mdl", 3), 0);
  }

  CHECK(app_instantiate_model(NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_instantiate_model(app, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_instantiate_model(NULL, model, NULL, NULL), BAD_ARG);
  CHECK(app_instantiate_model(app, model, NULL, NULL), OK);
  CHECK(app_instantiate_model(NULL, NULL, "inst0", NULL), BAD_ARG);
  CHECK(app_instantiate_model(app, NULL, "inst0", NULL), BAD_ARG);
  CHECK(app_instantiate_model(NULL, model, "inst0", NULL), BAD_ARG);
  CHECK(app_instantiate_model(app, model, "inst0", NULL), OK);
  CHECK(app_instantiate_model(NULL, NULL, NULL, &instance), BAD_ARG);
  CHECK(app_instantiate_model(app, NULL, NULL, &instance), BAD_ARG);
  CHECK(app_instantiate_model(NULL, model, NULL, &instance), BAD_ARG);
  CHECK(app_instantiate_model(app, model, NULL, &instance), OK);
  CHECK(app_instantiate_model(NULL, NULL, "inst1", &instance1), BAD_ARG);
  CHECK(app_instantiate_model(app, NULL, "inst1", &instance1), BAD_ARG);
  CHECK(app_instantiate_model(NULL, model, "inst1", &instance1), BAD_ARG);
  CHECK(app_instantiate_model(app, model, "inst1", &instance1), OK);

  CHECK(app_set_model_instance_name(NULL, NULL), BAD_ARG);
  CHECK(app_set_model_instance_name(instance, NULL), BAD_ARG);
  CHECK(app_set_model_instance_name(NULL, "my_inst0"), BAD_ARG);
  CHECK(app_set_model_instance_name(instance, "my_inst0"), OK);

  CHECK(app_model_instance_name(NULL, NULL), BAD_ARG);
  CHECK(app_model_instance_name(instance, NULL), BAD_ARG);
  CHECK(app_model_instance_name(NULL, &cstr), BAD_ARG);
  CHECK(app_model_instance_name(instance, &cstr), OK);
  CHECK(strcmp(cstr, "my_inst0"), 0);

  CHECK(app_get_model_instance(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance(app, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance(NULL, "my_inst0", NULL), BAD_ARG);
  CHECK(app_get_model_instance(app, "my_inst0", NULL), BAD_ARG);
  CHECK(app_get_model_instance(NULL, NULL, &instance), BAD_ARG);
  CHECK(app_get_model_instance(app, NULL, &instance), BAD_ARG);
  CHECK(app_get_model_instance(NULL, "my_inst0", &instance), BAD_ARG);
  CHECK(app_get_model_instance(app, "my_inst0", &instance), OK);
  CHECK(app_model_instance_name(instance, &cstr), OK);
  CHECK(strcmp(cstr, "my_inst0"), 0);

  CHECK(app_instantiate_model(app, model, "my_inst", NULL), OK);
  CHECK(app_instantiate_model(app, model, "inst1", NULL), OK);
  CHECK(app_instantiate_model(app, model, "myinst", NULL), OK);

  CHECK(app_model_instance_name_completion(NULL, NULL, 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, NULL, 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(NULL, "m", 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, "m", 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(NULL, NULL, 0, &len, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, NULL, 0, &len, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(NULL, "m", 0, &len, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, "m", 0, &len, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(NULL, NULL, 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, NULL, 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(NULL, "m", 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, "m", 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(NULL, NULL, 0, &len, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, NULL, 1, &len, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, NULL, 0, &len, &lst), OK);
  CHECK(len, 7);
  for(i = 1; i < len; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
  }
  CHECK(app_model_instance_name_completion(NULL, "m", 0, &len, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, "m", 0, &len, &lst), OK);
  CHECK(len, 7);
  for(i = 1; i < len; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
  }
  CHECK(app_model_instance_name_completion(app, "my", 2, &len, &lst), OK);
  CHECK(len < 7, true);
  CHECK(strncmp(lst[0], "my", 2), 0);
  for(i = 1; i < len; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
    CHECK(strncmp(lst[i], "my", 2), 0);
  }
  i = len;
  CHECK(app_model_instance_name_completion(app, "my_", 3, &len, &lst), OK);
  CHECK(len < i, true);
  CHECK(strncmp(lst[0], "my_", 3), 0);
  for(i = 1; i < len; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
    CHECK(strncmp(lst[i], "my_", 3), 0);
  }

  CHECK(app_model_instance_ref_get(NULL), BAD_ARG);
  CHECK(app_model_instance_ref_get(instance), OK);
  CHECK(app_model_instance_ref_put(NULL), BAD_ARG);
  CHECK(app_model_instance_ref_put(instance), OK);
  CHECK(app_model_instance_ref_put(instance), OK);

  CHECK(app_model_ref_get(NULL), BAD_ARG);
  CHECK(app_model_ref_get(model), OK);
  CHECK(app_model_ref_put(model), OK);
  CHECK(app_model_ref_put(model), OK);

  CHECK(app_cleanup(app), OK);
  CHECK(app_ref_put(app), OK);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;
}

