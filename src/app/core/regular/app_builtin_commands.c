#include "app/core/regular/app_builtin_commands.h"
#include "app/core/regular/app_c.h"
#include "app/core/regular/app_command_c.h"
#include "app/core/regular/app_model_c.h"
#include "app/core/regular/app_object.h"
#include "app/core/app_command.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "app/core/app_view.h"
#include "app/core/app_world.h"
#include "maths/simd/aosf33.h"
#include "maths/simd/aosf44.h"
#include "renderer/rdr_term.h"
#include "renderer/rdr.h"
#include "stdlib/sl.h"
#include "stdlib/sl_flat_map.h"
#include "stdlib/sl_flat_set.h"
#include "stdlib/sl_hash_table.h"
#include "sys/sys.h"
#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <string.h>

#define ARGVAL(argv, i) (argv)[i]->value_list[0]

/*******************************************************************************
 *
 * Command functions.
 *
 ******************************************************************************/
static void
cmd_exit
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv UNUSED)
{
  app->post_exit = true;
}

static void
cmd_load(struct app* app, size_t argc UNUSED, const struct app_cmdarg** argv)
{
  struct app_model* mdl = NULL;
  enum app_error app_err = APP_NO_ERROR;
  assert(app != NULL
      && argc == 3
      && argv != NULL
      && argv[0]->type == APP_CMDARG_STRING
      && argv[1]->type == APP_CMDARG_FILE
      && argv[2]->type == APP_CMDARG_STRING
      && argv[0]->count == 1
      && argv[1]->count == 1
      && argv[2]->count == 1);

  if(argv[1]->value_list[0].is_defined) {
    app_err = app_create_model
      (app,
       argv[1]->value_list[0].data.string,
       argv[2]->value_list[0].is_defined
        ? argv[2]->value_list[0].data.string
        : NULL,
       &mdl);
    if(app_err == APP_NO_ERROR) {
      APP_LOG_MSG
        (app->logger,
         "model loaded `%s'\n",
         argv[1]->value_list[0].data.string);
    } else {
      APP_LOG_ERR(app->logger, "model loading error\n");
    }
  } else {
    assert(false);
  }
}

static void
cmd_help(struct app* app, size_t argc UNUSED, const struct app_cmdarg** argv)
{
  enum app_error app_err = APP_NO_ERROR;
  assert(app != NULL
      && argc == 2
      && argv != NULL
      && argv[1]->type == APP_CMDARG_STRING
      && argv[1]->value_list[0].is_defined == true);

  rewind(app->cmd.stream);
  app_err = app_man_command
    (app,
     argv[1]->value_list[0].data.string,
     NULL,
     sizeof(app->cmd.scratch)/sizeof(char),
     app->cmd.scratch);
  if(app_err == APP_NO_ERROR)
    APP_LOG_MSG(app->logger, "%s", app->cmd.scratch);
}

static void
cmd_ls(struct app* app, size_t argc UNUSED, const struct app_cmdarg** argv)
{
  size_t len = 0;
  size_t i = 0;
  bool new_line = false;

  assert(app != NULL
      && argc == 4
      && argv != NULL
      && argv[1]->type == APP_CMDARG_LITERAL
      && argv[2]->type == APP_CMDARG_LITERAL
      && argv[3]->type == APP_CMDARG_LITERAL);

  if(argv[1]->value_list[0].is_defined) { /* commands */
    const char** name_list = NULL;
    SL(flat_set_buffer
      (app->cmd.name_set, &len, NULL, NULL, (void**)&name_list));
    for(i = 0; i < len; ++i) {
      APP_LOG_MSG(app->logger, "%s\n", name_list[i]);
    }
    APP_LOG_MSG(app->logger, "[total %zu commands]\n", len);
    new_line = true;
  }
  if(argv[3]->value_list[0].is_defined) { /* models */
    struct app_model** model_list = NULL;
    APP(get_model_list(app, &len, &model_list));
    if(new_line)
      APP_LOG_MSG(app->logger, "\n");
    for(i = 0; i < len; ++i) {
      const char* name = NULL;
      APP(model_name(model_list[i], &name));
      APP_LOG_MSG(app->logger, "%s\n", name);
    }
    APP_LOG_MSG(app->logger, "[total %zu models]\n", len);
    new_line = true;
  }
  if(argv[2]->value_list[0].is_defined) { /* model-instances */
    struct app_model_instance** instance_list = NULL;
    APP(get_model_instance_list(app, &len,&instance_list));
    if(new_line)
      APP_LOG_MSG(app->logger, "\n");
    for(i = 0; i < len; ++i) {
      const char* name = NULL;
      APP(model_instance_name(instance_list[i], &name));
      APP_LOG_MSG(app->logger, "%s\n", name);
    }
    APP_LOG_MSG(app->logger, "[total %zu model instances]\n", len);
  }
}

static void
cmd_clear
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv UNUSED)
{
  assert(app != NULL);
  RDR(clear_term(app->term.render_term, RDR_TERM_STDOUT));
}

static void
cmd_rename_model
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv)
{
  struct app_model* mdl = NULL;

  assert(app != NULL
      && argc == 3
      && argv != NULL
      && argv[0]->type == APP_CMDARG_STRING
      && argv[1]->type == APP_CMDARG_STRING
      && argv[2]->type == APP_CMDARG_STRING
      && argv[0]->count == 1
      && argv[1]->count == 1
      && argv[2]->count == 1);

  APP(get_model(app, argv[1]->value_list[0].data.string, &mdl));
  if(mdl) {
    APP(set_model_name(mdl, argv[2]->value_list[0].data.string));
  } else {
    APP_LOG_ERR
      (app->logger,
       "the model `%s' does not exist\n",
       argv[1]->value_list[0].data.string);
  }
}

static void
cmd_rename_instance
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv)
{
  struct app_model_instance* instance = NULL;

  assert(app != NULL
      && argc == 3
      && argv != NULL
      && argv[0]->type == APP_CMDARG_STRING
      && argv[1]->type == APP_CMDARG_STRING
      && argv[2]->type == APP_CMDARG_STRING
      && argv[0]->count == 1
      && argv[1]->count == 1
      && argv[2]->count == 1);

  APP(get_model_instance(app, argv[1]->value_list[0].data.string, &instance));
  if(instance) {
    APP(set_model_instance_name(instance, argv[2]->value_list[0].data.string));
  } else {
    APP_LOG_ERR
      (app->logger,
       "the instance `%s' does not exist\n",
       argv[1]->value_list[0].data.string);
  }
}

static void
cmd_spawn(struct app* app, size_t argc UNUSED, const struct app_cmdarg** argv)
{
  assert(app != NULL
      && argc == 3
      && argv != NULL
      && argv[0]->type == APP_CMDARG_STRING
      && argv[1]->type == APP_CMDARG_STRING
      && argv[2]->type == APP_CMDARG_STRING
      && argv[0]->count == 1
      && argv[1]->count == 1
      && argv[2]->count == 1);

  if(argv[1]->value_list[0].is_defined == true) {
    struct app_object* obj = NULL;
    APP(get_object
      (app, APP_MODEL, argv[1]->value_list[0].data.string, &obj));
    if(obj == NULL) {
      APP_LOG_ERR
        (app->logger,
         "the model `%s' does not exist\n",
         argv[1]->value_list[0].data.string);
    } else {
      struct app_model* mdl = app_object_to_model(obj);
      struct app_model_instance* instance = NULL;
      enum app_error app_err = APP_NO_ERROR;

      app_err = app_instantiate_model
        (app,
         mdl,
         argv[2]->value_list[0].is_defined
          ? argv[2]->value_list[0].data.string
          : NULL,
         &instance);
      if(app_err != APP_NO_ERROR) {
        APP_LOG_ERR
          (app->logger,
           "error instantiating the model `%s': %s\n",
           argv[1]->value_list[0].data.string,
           app_error_string(app_err));
      } else {
        app_err = app_world_add_model_instances(app->world, 1, &instance);
        if(app_err != APP_NO_ERROR) {
          const char* cstr = NULL;
          APP(model_instance_name(instance, &cstr));
          APP_LOG_ERR
            (app->logger,
             "error adding the instance `%s' to the world: %s\n",
             cstr,
             app_error_string(app_err));
          APP(model_instance_ref_put(instance));
        }
      }
    }
  }
}

static void
cmd_translate
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv)
{
  size_t nb_defined_flags = 0;
  enum {
    CMD_NAME,
    EYE_SPACE_FLAG,
    LOCAL_SPACE_FLAG,
    WORLD_SPACE_FLAG,
    INSTANCE_NAME,
    TRANS_X,
    TRANS_Y,
    TRANS_Z,
    ARGC
  };

  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[EYE_SPACE_FLAG]->type == APP_CMDARG_LITERAL
      && argv[LOCAL_SPACE_FLAG]->type == APP_CMDARG_LITERAL
      && argv[WORLD_SPACE_FLAG]->type == APP_CMDARG_LITERAL
      && argv[INSTANCE_NAME]->type == APP_CMDARG_STRING
      && argv[TRANS_X]->type == APP_CMDARG_FLOAT
      && argv[TRANS_Y]->type == APP_CMDARG_FLOAT
      && argv[TRANS_Z]->type == APP_CMDARG_FLOAT
      && argv[CMD_NAME]->count == 1
      && argv[EYE_SPACE_FLAG]->count == 1
      && argv[LOCAL_SPACE_FLAG]->count == 1
      && argv[WORLD_SPACE_FLAG]->count == 1
      && argv[INSTANCE_NAME]->count == 1
      && argv[TRANS_X]->count == 1
      && argv[TRANS_Y]->count == 1
      && argv[TRANS_Z]->count == 1);

  nb_defined_flags =
      (ARGVAL(argv, EYE_SPACE_FLAG).is_defined == true)
    + (ARGVAL(argv, LOCAL_SPACE_FLAG).is_defined == true)
    + (ARGVAL(argv, WORLD_SPACE_FLAG).is_defined == true);

  if(nb_defined_flags != 1) {
    if(nb_defined_flags == 0) {
      APP_LOG_ERR(app->logger, "no translation space defined");
    } else {
      APP_LOG_ERR(app->logger, "only one translation space must be defined");
    }
  } else {
    struct app_model_instance* instance = NULL;
    const char* instance_name = ARGVAL(argv, INSTANCE_NAME).data.string;

    APP(get_model_instance(app, instance_name, &instance));
    if(instance == NULL) {
      APP_LOG_ERR
        (app->logger, "the instance `%s' does not exist\n", instance_name);
    } else {
      const float trans[3] = {
        [0] = ARGVAL(argv, TRANS_X).is_defined
            ? ARGVAL(argv, TRANS_X).data.real
            : 0.f,
        [1] = ARGVAL(argv, TRANS_Y).is_defined
            ? ARGVAL(argv, TRANS_Y).data.real
            : 0.f,
        [2] = ARGVAL(argv, TRANS_Z).is_defined
            ? ARGVAL(argv, TRANS_Z).data.real
            : 0.f
      };

      if(trans[0] != 0.f || trans[1] != 0.f || trans[2] != 0.f) {
        if(ARGVAL(argv, LOCAL_SPACE_FLAG).is_defined) { /* object space */
          APP(translate_model_instance(instance, true, trans));
        } else if(ARGVAL(argv, WORLD_SPACE_FLAG).is_defined) { /* world space */
          APP(translate_model_instance(instance, false, trans));
        } else { /* eye space */
          const struct aosf44* view_transform = NULL;
          struct aosf33 f33;
          struct app_view* view = NULL;
          ALIGN(16) float tmp[4];
          vf4_t vec;
          assert(ARGVAL(argv, EYE_SPACE_FLAG).is_defined);

          APP(get_main_view(app, &view));
          APP(get_raw_view_transform(view, &view_transform));
          f33.c0 = view_transform->c0;
          f33.c1 = view_transform->c1;
          f33.c2 = view_transform->c2;

          vec = aosf33_inverse(&f33, &f33);
          assert((vf4_store(tmp, vec), tmp[0] != 0.f));
          vec = vf4_set(trans[0], trans[1], trans[2], 1.f);
          vec = aosf33_mulf3(&f33, vec);
          vf4_store(tmp, vec);

          APP(translate_model_instance(instance, false, tmp));
        }
      }
    }
  }
}

static void
cmd_rotate(struct app* app, size_t argc UNUSED, const struct app_cmdarg** argv)
{
  size_t nb_defined_flags = 0;
  enum {
    CMD_NAME,
    EYE_SPACE_FLAG,
    LOCAL_SPACE_FLAG,
    WORLD_SPACE_FLAG,
    INSTANCE_NAME,
    PITCH_ROTATION,
    YAW_ROTATION,
    ROLL_ROTATION,
    RADIAN_FLAG,
    ARGC
  };

  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[EYE_SPACE_FLAG]->type == APP_CMDARG_LITERAL
      && argv[LOCAL_SPACE_FLAG]->type == APP_CMDARG_LITERAL
      && argv[WORLD_SPACE_FLAG]->type == APP_CMDARG_LITERAL
      && argv[INSTANCE_NAME]->type == APP_CMDARG_STRING
      && argv[PITCH_ROTATION]->type == APP_CMDARG_FLOAT
      && argv[YAW_ROTATION]->type == APP_CMDARG_FLOAT
      && argv[ROLL_ROTATION]->type == APP_CMDARG_FLOAT
      && argv[RADIAN_FLAG]->type == APP_CMDARG_LITERAL
      && argv[CMD_NAME]->count == 1
      && argv[EYE_SPACE_FLAG]->count == 1
      && argv[LOCAL_SPACE_FLAG]->count == 1
      && argv[WORLD_SPACE_FLAG]->count == 1
      && argv[INSTANCE_NAME]->count == 1
      && argv[PITCH_ROTATION]->count == 1
      && argv[YAW_ROTATION]->count == 1
      && argv[ROLL_ROTATION]->count == 1
      && argv[RADIAN_FLAG]->count == 1);

  nb_defined_flags =
      (ARGVAL(argv, EYE_SPACE_FLAG).is_defined == true)
    + (ARGVAL(argv, LOCAL_SPACE_FLAG).is_defined == true)
    + (ARGVAL(argv, WORLD_SPACE_FLAG).is_defined == true);

  if(nb_defined_flags != 1) {
    if(nb_defined_flags == 0) {
      APP_LOG_ERR(app->logger, "no rotation space defined");
    } else {
      APP_LOG_ERR(app->logger, "only one rotation space must be defined");
    }
  } else {
    struct app_model_instance* instance = NULL;
    const char* instance_name = ARGVAL(argv, 4).data.string;

    APP(get_model_instance(app, instance_name, &instance));
    if(instance == NULL) {
      APP_LOG_ERR
        (app->logger, "the instance `%s' does not exist\n", instance_name);
    } else {
      float rot[3] = {
        [0] = ARGVAL(argv, PITCH_ROTATION).is_defined
            ? ARGVAL(argv, PITCH_ROTATION).data.real
            : 0.f,
        [1] = ARGVAL(argv, YAW_ROTATION).is_defined
            ? ARGVAL(argv, YAW_ROTATION).data.real
            : 0.f,
        [2] = ARGVAL(argv, ROLL_ROTATION).is_defined
            ? ARGVAL(argv, ROLL_ROTATION).data.real
            : 0.f
      };

      if(rot[0] != 0.f || rot[1] != 0.f || rot[2] != 0.f) {
        if(ARGVAL(argv, RADIAN_FLAG).is_defined == false) {
          rot[0] = DEG2RAD(rot[0]);
          rot[1] = DEG2RAD(rot[1]);
          rot[2] = DEG2RAD(rot[2]);
        }
        if(ARGVAL(argv, LOCAL_SPACE_FLAG).is_defined) {
          APP(rotate_model_instance(instance, true, rot));
        } else if(ARGVAL(argv, WORLD_SPACE_FLAG).is_defined) {
          APP(rotate_model_instance(instance, false, rot));
        } else {
          struct aosf44 inv_view_4x4;
          struct aosf44 tmp_4x4;
          struct aosf33 rot_3x3;
          struct aosf33 tmp_3x3;
          vf4_t vec;
          UNUSED ALIGN(16) float tmp[4];
          const struct aosf44* view_4x4 = NULL;
          struct app_view* view = NULL;
          assert(ARGVAL(argv, EYE_SPACE_FLAG).is_defined);

          /* Get the view matrix and its inverse. */
          APP(get_main_view(app, &view));
          APP(get_raw_view_transform(view, &view_4x4));
          vec = aosf44_inverse(&inv_view_4x4, view_4x4);
          assert((vf4_store(tmp, vec), tmp[0] != 0.f));

          /* Compute the matrix to apply to the model. */
          aosf33_rotation(&rot_3x3, rot[0], rot[1], rot[2]);
          aosf33_set(&tmp_3x3, view_4x4->c0, view_4x4->c1, view_4x4->c2);
          aosf33_mulf33(&tmp_3x3, &rot_3x3, &tmp_3x3);
          aosf44_set(&tmp_4x4, tmp_3x3.c0, tmp_3x3.c1, tmp_3x3.c2,view_4x4->c3);
          aosf44_mulf44(&tmp_4x4, &inv_view_4x4, &tmp_4x4);

          APP(transform_model_instance(instance, true, &tmp_4x4));
        }
      }
    }
  }
}

/*******************************************************************************
 *
 * Builtin commands registration.
 *
 ******************************************************************************/
enum app_error
app_setup_builtin_commands(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  assert(app);

  #define CALL(func) if(APP_NO_ERROR != (app_err = func)) goto error

  CALL(app_add_command
    (app, "clear", cmd_clear, NULL, NULL, "clear the terminal screen"));
  CALL(app_add_command
    (app, "exit", cmd_exit, NULL, NULL, "cause the application to exit"));
  CALL(app_add_command
    (app, "help", cmd_help, app_command_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING(NULL, NULL, "<command>", NULL, 1, 1, NULL),
      APP_CMDARG_END),
     "give command informations"));
  CALL(app_add_command
    (app, "ls", cmd_ls, NULL,
     APP_CMDARGV
     (APP_CMDARG_APPEND_LITERAL
      ("c", "commands", "list the registered commands", 0, 1),
      APP_CMDARG_APPEND_LITERAL
      ("i", "model-instances", "list the spawned model instances", 0, 1),
      APP_CMDARG_APPEND_LITERAL
      ("m", "models", "list the loaded models", 0, 1),
      APP_CMDARG_END),
     "list application contents"));
  CALL(app_add_command
    (app, "load", cmd_load, NULL,
     APP_CMDARGV
     (APP_CMDARG_APPEND_FILE("m", "model", "<path>", NULL, 1, 1),
      APP_CMDARG_APPEND_STRING("n", "name", "<name>", NULL, 0, 1, NULL),
      APP_CMDARG_END),
     "load resources"));
  CALL(app_add_command
    (app, "rename", cmd_rename_model, app_model_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING("m", "model", "<model>", NULL, 1, 1, NULL),
      APP_CMDARG_APPEND_STRING(NULL, NULL, "<name>", NULL, 1, 1, NULL),
      APP_CMDARG_END),
     "rename model"));
  CALL(app_add_command
    (app, "rename", cmd_rename_instance, app_model_instance_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING("i", "instance", "<instance>", NULL, 1, 1, NULL),
      APP_CMDARG_APPEND_STRING(NULL, NULL, "<name>", NULL, 1, 1, NULL),
      APP_CMDARG_END),
     "rename instance"));
  CALL(app_add_command
    (app, "rotate", cmd_rotate, app_model_instance_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_LITERAL("e", "eye", "eye space rotation", 0, 1),
      APP_CMDARG_APPEND_LITERAL("l", "local", "object space rotation", 0, 1),
      APP_CMDARG_APPEND_LITERAL("w", "world", "world space rotation", 0, 1),
      APP_CMDARG_APPEND_STRING
        ("i", "instance", "<name>", "instance to rotate", 1, 1, NULL),
      APP_CMDARG_APPEND_FLOAT
        ("x", NULL, "<real>", "pitch angle", 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_FLOAT
        ("y", NULL, "<real>", "yaw angle", 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_FLOAT
        ("z", NULL, "<real>", "roll angle", 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_LITERAL
        ("r", "radian",
         "the input angles are expressed into radians rather than in degrees",
         0, 1),
      APP_CMDARG_END),
     "rotate a model instance"));
  CALL(app_add_command
    (app, "spawn", cmd_spawn, app_model_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING
        ("m", "model", "<model>",
         "name of the model from which an instance is spawned",
         1, 1, NULL),
      APP_CMDARG_APPEND_STRING
        ("n", "name", "<name>",
         "name of the spawned instance",
         0, 1, NULL),
      APP_CMDARG_END),
     "spawn an instance into the world"));
  CALL(app_add_command
    (app, "translate", cmd_translate, app_model_instance_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_LITERAL("e", "eye", "eye space translation", 0, 1),
      APP_CMDARG_APPEND_LITERAL("l", "local", "object space translation", 0, 1),
      APP_CMDARG_APPEND_LITERAL("w", "world", "local space translation", 0, 1),
      APP_CMDARG_APPEND_STRING
        ("i", "instance", "<name>", "instance to translate", 1, 1, NULL),
      APP_CMDARG_APPEND_FLOAT
        ("x", NULL, "<real>", NULL, 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_FLOAT
        ("y", NULL, "<real>", NULL, 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_FLOAT
        ("z", NULL, "<real>", NULL, 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_END),
     "translate a model instance"));

  #undef CALL

exit:
  return app_err;
error:
  goto exit;
}

#undef ARGVAL

