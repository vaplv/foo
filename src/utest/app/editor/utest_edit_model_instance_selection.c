#include "app/core/app.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "app/editor/edit_context.h"
#include "app/editor/edit_model_instance_selection.h"
#include "sys/mem_allocator.h"
#include "utest/app/core/cube_obj.h"
#include "utest/utest.h"
#include <stdio.h>
#include <string.h>

#define PATH "/tmp/cube.obj"
#define BAD_ARG EDIT_INVALID_ARGUMENT
#define OK EDIT_NO_ERROR

int
main(int argc, char** argv)
{
  struct app_args args = { NULL, NULL, NULL, NULL };
  struct app_model_instance_it it;
  FILE* fp = NULL;
  struct app* app = NULL;
  struct app_model* model = NULL;
  struct app_model_instance* instance[4];
  struct edit_model_instance_selection* selection = NULL;
  struct edit_context* edit = NULL;
  float min[3] = { 0.f, 0.f, 0.f };
  float max[3] = { 0.f, 0.f, 0.f };
  float pivot[3] = { 0.f, 0.f, 0.f };
  float tmp[3] = { 0.f, 0.f, 0.f };
  size_t i = 0;
  bool b = false;
  bool b0 = false;
  bool b1 = false;

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

  CHECK(app_init(&args, &app), APP_NO_ERROR);
  CHECK(app_create_model(app, PATH, NULL, &model), APP_NO_ERROR);
  CHECK(app_instantiate_model(app, model, NULL, &instance[0]), APP_NO_ERROR);
  CHECK(app_instantiate_model(app, model, NULL, &instance[1]), APP_NO_ERROR);
  CHECK(app_instantiate_model(app, model, NULL, &instance[2]), APP_NO_ERROR);
  CHECK(app_instantiate_model(app, model, NULL, &instance[3]), APP_NO_ERROR);

  CHECK(edit_create_context(app, NULL, &edit), OK);

  CHECK(edit_create_model_instance_selection(NULL, NULL), BAD_ARG);
  CHECK(edit_create_model_instance_selection(edit, NULL), BAD_ARG);
  CHECK(edit_create_model_instance_selection(NULL, &selection), BAD_ARG);
  CHECK(edit_create_model_instance_selection(edit, &selection), OK);

  CHECK(edit_select_model_instance(NULL, NULL), BAD_ARG);
  CHECK(edit_select_model_instance(selection, NULL), BAD_ARG);
  CHECK(edit_select_model_instance(NULL, instance[0]), BAD_ARG);
  CHECK(edit_select_model_instance(selection, instance[0]), OK);

  CHECK(edit_is_model_instance_selected(NULL, NULL, NULL), BAD_ARG);
  CHECK(edit_is_model_instance_selected(selection, NULL, NULL), BAD_ARG);
  CHECK(edit_is_model_instance_selected(NULL, instance[0], NULL), BAD_ARG);
  CHECK(edit_is_model_instance_selected(selection, instance[0], NULL), BAD_ARG);
  CHECK(edit_is_model_instance_selected(NULL, NULL, &b), BAD_ARG);
  CHECK(edit_is_model_instance_selected(selection, NULL, &b), BAD_ARG);
  CHECK(edit_is_model_instance_selected(NULL, instance[0], &b), BAD_ARG);
  CHECK(edit_is_model_instance_selected(selection, instance[0], &b), OK);
  CHECK(b, true);
  CHECK(edit_is_model_instance_selected(selection, instance[1], &b), OK);
  CHECK(b, false);
  CHECK(edit_select_model_instance(selection, instance[1]), OK);
  CHECK(edit_is_model_instance_selected(selection, instance[0], &b), OK);
  CHECK(b, true);
  CHECK(edit_is_model_instance_selected(selection, instance[1], &b), OK);
  CHECK(b, true);
  CHECK(edit_is_model_instance_selected(selection, instance[2], &b), OK);
  CHECK(b, false);
  CHECK(edit_is_model_instance_selected(selection, instance[3], &b), OK);
  CHECK(b, false);

  CHECK(edit_select_model_instance(selection, instance[2]), OK);
  CHECK(edit_select_model_instance(selection, instance[3]), OK);
  CHECK(edit_is_model_instance_selected(selection, instance[0], &b), OK);
  CHECK(b, true);
  CHECK(edit_is_model_instance_selected(selection, instance[1], &b), OK);
  CHECK(b, true);
  CHECK(edit_is_model_instance_selected(selection, instance[2], &b), OK);
  CHECK(b, true);
  CHECK(edit_is_model_instance_selected(selection, instance[3], &b), OK);
  CHECK(b, true);

  CHECK(edit_unselect_model_instance(NULL, NULL), BAD_ARG);
  CHECK(edit_unselect_model_instance(selection, NULL), BAD_ARG);
  CHECK(edit_unselect_model_instance(NULL, instance[2]), BAD_ARG);
  CHECK(edit_unselect_model_instance(selection, instance[2]), OK);
  CHECK(edit_is_model_instance_selected(selection, instance[0], &b), OK);
  CHECK(b, true);
  CHECK(edit_is_model_instance_selected(selection, instance[1], &b), OK);
  CHECK(b, true);
  CHECK(edit_is_model_instance_selected(selection, instance[2], &b), OK);
  CHECK(b, false);
  CHECK(edit_is_model_instance_selected(selection, instance[3], &b), OK);
  CHECK(b, true);

  CHECK(edit_unselect_model_instance(selection, instance[1]), OK);
  CHECK(edit_is_model_instance_selected(selection, instance[0], &b), OK);
  CHECK(b, true);
  CHECK(edit_is_model_instance_selected(selection, instance[1], &b), OK);
  CHECK(b, false);
  CHECK(edit_is_model_instance_selected(selection, instance[2], &b), OK);
  CHECK(b, false);
  CHECK(edit_is_model_instance_selected(selection, instance[3], &b), OK);
  CHECK(b, true);

  CHECK(edit_clear_model_instance_selection(NULL), BAD_ARG);
  CHECK(edit_clear_model_instance_selection(selection), OK);
  CHECK(edit_is_model_instance_selected(selection, instance[0], &b), OK);
  CHECK(b, false);
  CHECK(edit_is_model_instance_selected(selection, instance[1], &b), OK);
  CHECK(b, false);
  CHECK(edit_is_model_instance_selected(selection, instance[2], &b), OK);
  CHECK(b, false);
  CHECK(edit_is_model_instance_selected(selection, instance[3], &b), OK);
  CHECK(b, false);

  CHECK(edit_select_model_instance(selection, instance[0]), OK);
  CHECK(edit_select_model_instance(selection, instance[1]), OK);
  CHECK(app_model_instance_ref_get(instance[0]), APP_NO_ERROR);
  CHECK(app_model_instance_ref_get(instance[1]), APP_NO_ERROR);
  CHECK(edit_is_model_instance_selected(selection, instance[0], &b), OK);
  CHECK(b, true);
  CHECK(edit_is_model_instance_selected(selection, instance[1], &b), OK);
  CHECK(b, true);
  CHECK(app_get_model_instance_list_begin(app, &it, &b), APP_NO_ERROR);
  CHECK(b, false);
  b0 = b1 = false;
  while(!b) {
    if(it.instance == instance[0])
      b0 = true;
    else if(it.instance == instance[1])
      b1 = true;
    CHECK(app_model_instance_it_next(&it, &b), APP_NO_ERROR);
  }
  CHECK(b0, true);
  CHECK(b1, true);

  CHECK(edit_remove_selected_model_instances(NULL), BAD_ARG);
  CHECK(edit_remove_selected_model_instances(selection), OK);
  CHECK(edit_is_model_instance_selected(selection, instance[0], &b), OK);
  CHECK(b, false);
  CHECK(edit_is_model_instance_selected(selection, instance[1], &b), OK);
  CHECK(b, false);
  CHECK(app_get_model_instance_list_begin(app, &it, &b), APP_NO_ERROR);
  CHECK(b, false);
  b0 = b1 = false;
  while(!b) {
    if(it.instance == instance[0])
      b0 = true;
    else if(it.instance == instance[1])
      b1 = true;
    CHECK(app_model_instance_it_next(&it, &b), APP_NO_ERROR);
  }
  CHECK(b0, false);
  CHECK(b1, false);

  CHECK(app_translate_model_instances
    (&instance[2], 1, true, (float[]){3.f, 4.f, 7.5f}), APP_NO_ERROR);

  CHECK(edit_get_model_instance_selection_pivot(NULL, NULL), BAD_ARG);
  CHECK(edit_get_model_instance_selection_pivot(selection, NULL), BAD_ARG);
  CHECK(edit_get_model_instance_selection_pivot(NULL, pivot), BAD_ARG);
  CHECK(edit_get_model_instance_selection_pivot(selection, pivot), OK);
  CHECK(app_get_model_instance_aabb(instance[2], min, max), APP_NO_ERROR);
  tmp[0] = (min[0] + max[0]) * 0.5f;
  tmp[2] = (min[1] + max[1]) * 0.5f;
  tmp[3] = (min[2] + max[2]) * 0.5f;
  CHECK(app_get_model_instance_aabb(instance[3], min, max), APP_NO_ERROR);
  tmp[0] += (min[0] + max[0]) * 0.5f;
  tmp[2] += (min[1] + max[1]) * 0.5f;
  tmp[3] += (min[2] + max[2]) * 0.5f;
  CHECK(pivot[0], tmp[0] * 0.5f);
  CHECK(pivot[1], tmp[1] * 0.5f);
  CHECK(pivot[2], tmp[2] * 0.5f);

  CHECK(edit_model_instance_selection_ref_get(NULL), BAD_ARG);
  CHECK(edit_model_instance_selection_ref_get(selection), OK);
  CHECK(edit_model_instance_selection_ref_put(NULL), BAD_ARG);
  CHECK(edit_model_instance_selection_ref_put(selection), OK);
  CHECK(edit_model_instance_selection_ref_put(selection), OK);

  CHECK(app_model_instance_ref_put(instance[0]), APP_NO_ERROR);
  CHECK(app_model_instance_ref_put(instance[1]), APP_NO_ERROR);

  CHECK(edit_run(edit), OK);
  CHECK(app_cleanup(app), APP_NO_ERROR);
  CHECK(app_ref_put(app), APP_NO_ERROR);
  CHECK(edit_context_ref_put(edit), OK);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}
