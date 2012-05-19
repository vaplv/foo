#include "app/editor/regular/edit_context_c.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_move_commands.h"
#include "app/editor/regular/edit_model_instance_selection.h"
#include "app/editor/edit_context.h"
#include "app/editor/edit_error.h"
#include "app/core/app.h"
#include "app/core/app_model_instance.h"
#include "app/core/app_command.h"
#include "app/core/app_view.h"
#include "maths/simd/aosf33.h"
#include "maths/simd/aosf44.h"
#include "sys/math.h"
#include <float.h>
#include <stdbool.h>

/*******************************************************************************
 *
 * Command function.
 *
 ******************************************************************************/
static void
mv
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data UNUSED)
{
  struct app_model_instance* instance = NULL;
  const char* name = NULL;
  enum { CMD_NAME, INSTANCE_NAME, POS_X, POS_Y, POS_Z, ARGC };

  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[INSTANCE_NAME]->type == APP_CMDARG_STRING
      && argv[POS_X]->type == APP_CMDARG_FLOAT
      && argv[POS_Y]->type == APP_CMDARG_FLOAT
      && argv[POS_Z]->type == APP_CMDARG_FLOAT);

  name = EDIT_CMD_ARGVAL(argv, INSTANCE_NAME).data.string;
  APP(get_model_instance(app, name, &instance));
  if(instance == NULL) {
    APP(log(app, APP_LOG_ERROR, "the instance `%s' does not exist\n", name));
  } else {
   ALIGN(16) float tmp[4];
    const struct aosf44* f44 = NULL;

    APP(get_raw_model_instance_transform(instance, &f44));
    vf4_store(tmp, f44->c3);

    if((!EDIT_CMD_ARGVAL(argv, POS_X).is_defined)
     & (!EDIT_CMD_ARGVAL(argv, POS_Y).is_defined)
     & (!EDIT_CMD_ARGVAL(argv, POS_Z).is_defined)) {
      APP(log(app, APP_LOG_INFO,
        "%s: %s position: %f %f %f\n",
        EDIT_CMD_ARGVAL(argv, CMD_NAME).data.string,
        EDIT_CMD_ARGVAL(argv, INSTANCE_NAME).data.string,
        tmp[0], tmp[1], tmp[2]));
    } else {
      if(EDIT_CMD_ARGVAL(argv, POS_X).is_defined)
        tmp[0] = EDIT_CMD_ARGVAL(argv, POS_X).data.real;
      if(EDIT_CMD_ARGVAL(argv, POS_Y).is_defined)
        tmp[1] = EDIT_CMD_ARGVAL(argv, POS_Y).data.real;
      if(EDIT_CMD_ARGVAL(argv, POS_Z).is_defined)
        tmp[2] = EDIT_CMD_ARGVAL(argv, POS_Z).data.real;

      APP(move_model_instances(&instance, 1, tmp));
    }
  }
}

static void
mv_selection
  (struct app* app UNUSED,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data)
{
  struct edit_context* ctxt = data;
  float pivot[3] = { 0.f, 0.f, 0.f };
  float translation[3] = { 0.f, 0.f, 0.f };
  enum { CMD_NAME, SELECTION_FLAG, POS_X, POS_Y, POS_Z, ARGC };

  assert(app != NULL
      && data != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[SELECTION_FLAG]->type == APP_CMDARG_LITERAL
      && argv[POS_X]->type == APP_CMDARG_FLOAT
      && argv[POS_Y]->type == APP_CMDARG_FLOAT
      && argv[POS_Z]->type == APP_CMDARG_FLOAT);

  EDIT(get_model_instance_selection_pivot(ctxt, pivot));

  if(EDIT_CMD_ARGVAL(argv, POS_X).is_defined)
    translation[0] = EDIT_CMD_ARGVAL(argv, POS_X).data.real - pivot[0];
  if(EDIT_CMD_ARGVAL(argv, POS_Y).is_defined)
    translation[1] = EDIT_CMD_ARGVAL(argv, POS_Y).data.real - pivot[1];
  if(EDIT_CMD_ARGVAL(argv, POS_Z).is_defined)
    translation[2] = EDIT_CMD_ARGVAL(argv, POS_Z).data.real - pivot[2];

  EDIT(translate_model_instance_selection(ctxt, false, translation));
}

static void
translate
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data UNUSED)
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
      && argv[TRANS_Z]->type == APP_CMDARG_FLOAT);

  nb_defined_flags =
      (EDIT_CMD_ARGVAL(argv, EYE_SPACE_FLAG).is_defined == true)
    + (EDIT_CMD_ARGVAL(argv, LOCAL_SPACE_FLAG).is_defined == true)
    + (EDIT_CMD_ARGVAL(argv, WORLD_SPACE_FLAG).is_defined == true);

  if(nb_defined_flags != 1) {
    if(nb_defined_flags == 0) {
      APP(log(app, APP_LOG_ERROR, "missing coordinate system\n"));
    } else {
      APP(log(app, APP_LOG_ERROR,
        "only one coordinate system must be defined\n"));
    }
  } else {
    struct app_model_instance* instance = NULL;
    const char* name = EDIT_CMD_ARGVAL(argv, INSTANCE_NAME).data.string;

    APP(get_model_instance(app, name, &instance));
    if(instance == NULL) {
      APP(log(app, APP_LOG_ERROR, "the instance `%s' does not exist\n", name));
    } else {
      const float trans[3] = {
        [0] = EDIT_CMD_ARGVAL(argv, TRANS_X).is_defined
            ? EDIT_CMD_ARGVAL(argv, TRANS_X).data.real
            : 0.f,
        [1] = EDIT_CMD_ARGVAL(argv, TRANS_Y).is_defined
            ? EDIT_CMD_ARGVAL(argv, TRANS_Y).data.real
            : 0.f,
        [2] = EDIT_CMD_ARGVAL(argv, TRANS_Z).is_defined
            ? EDIT_CMD_ARGVAL(argv, TRANS_Z).data.real
            : 0.f
      };

      if(trans[0] != 0.f || trans[1] != 0.f || trans[2] != 0.f) {
        if(EDIT_CMD_ARGVAL(argv, LOCAL_SPACE_FLAG).is_defined) { /* object space */
          APP(translate_model_instances(&instance, 1, true, trans));
        } else if(EDIT_CMD_ARGVAL(argv, WORLD_SPACE_FLAG).is_defined) { /* world space */
          APP(translate_model_instances(&instance, 1, false, trans));
        } else { /* eye space */
          const struct aosf44* view_transform = NULL;
          struct aosf33 f33;
          struct app_view* view = NULL;
          ALIGN(16) float tmp[4];
          vf4_t vec;
          assert(EDIT_CMD_ARGVAL(argv, EYE_SPACE_FLAG).is_defined);

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

          APP(translate_model_instances(&instance, 1, false, tmp));
        }
      }
    }
  }
}

static void
rotate
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data UNUSED)
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
      && argv[RADIAN_FLAG]->type == APP_CMDARG_LITERAL);

  nb_defined_flags =
      (EDIT_CMD_ARGVAL(argv, EYE_SPACE_FLAG).is_defined == true)
    + (EDIT_CMD_ARGVAL(argv, LOCAL_SPACE_FLAG).is_defined == true)
    + (EDIT_CMD_ARGVAL(argv, WORLD_SPACE_FLAG).is_defined == true);

  if(nb_defined_flags != 1) {
    if(nb_defined_flags == 0) {
      APP(log(app, APP_LOG_ERROR, "missing coordinate system\n"));
    } else {
      APP(log(app, APP_LOG_ERROR,
        "only one coordinate system must be defined\n"));
    }
  } else {
    struct app_model_instance* instance = NULL;
    const char* name = EDIT_CMD_ARGVAL(argv, INSTANCE_NAME).data.string;

    APP(get_model_instance(app, name, &instance));
    if(instance == NULL) {
      APP(log(app, APP_LOG_ERROR, "the instance `%s' does not exist\n", name));
    } else {
      float rot[3] = {
        [0] = EDIT_CMD_ARGVAL(argv, PITCH_ROTATION).is_defined
            ? EDIT_CMD_ARGVAL(argv, PITCH_ROTATION).data.real
            : 0.f,
        [1] = EDIT_CMD_ARGVAL(argv, YAW_ROTATION).is_defined
            ? EDIT_CMD_ARGVAL(argv, YAW_ROTATION).data.real
            : 0.f,
        [2] = EDIT_CMD_ARGVAL(argv, ROLL_ROTATION).is_defined
            ? EDIT_CMD_ARGVAL(argv, ROLL_ROTATION).data.real
            : 0.f
      };

      if(rot[0] != 0.f || rot[1] != 0.f || rot[2] != 0.f) {
        if(EDIT_CMD_ARGVAL(argv, RADIAN_FLAG).is_defined == false) {
          rot[0] = DEG2RAD(rot[0]);
          rot[1] = DEG2RAD(rot[1]);
          rot[2] = DEG2RAD(rot[2]);
        }
        if(EDIT_CMD_ARGVAL(argv, LOCAL_SPACE_FLAG).is_defined) {
          APP(rotate_model_instances(&instance, 1, true, rot));
        } else if(EDIT_CMD_ARGVAL(argv, WORLD_SPACE_FLAG).is_defined) {
          APP(rotate_model_instances(&instance, 1, false, rot));
        } else {
          struct aosf44 inv_view_4x4;
          struct aosf44 tmp_4x4;
          struct aosf33 rot_3x3;
          struct aosf33 tmp_3x3;
          vf4_t vec;
          UNUSED ALIGN(16) float tmp[4];
          const struct aosf44* view_4x4 = NULL;
          struct app_view* view = NULL;
          assert(EDIT_CMD_ARGVAL(argv, EYE_SPACE_FLAG).is_defined);

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

          APP(transform_model_instances(&instance, 1, true, &tmp_4x4));
        }
      }
    }
  }
}

static void
scale
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data UNUSED)
{
  size_t nb_defined_flags = 0;
  enum {
    CMD_NAME,
    EYE_SPACE_FLAG,
    LOCAL_SPACE_FLAG,
    WORLD_SPACE_FLAG,
    INSTANCE_NAME,
    SCALE_X,
    SCALE_Y,
    SCALE_Z,
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
      && argv[SCALE_X]->type == APP_CMDARG_FLOAT
      && argv[SCALE_Y]->type == APP_CMDARG_FLOAT
      && argv[SCALE_Z]->type == APP_CMDARG_FLOAT);

  nb_defined_flags =
      (EDIT_CMD_ARGVAL(argv, EYE_SPACE_FLAG).is_defined == true)
    + (EDIT_CMD_ARGVAL(argv, LOCAL_SPACE_FLAG).is_defined == true)
    + (EDIT_CMD_ARGVAL(argv, WORLD_SPACE_FLAG).is_defined == true);

  if(nb_defined_flags != 1) {
    if(nb_defined_flags == 0) {
      APP(log(app, APP_LOG_ERROR, "missing coordinate system\n"));
    } else {
      APP(log(app, APP_LOG_ERROR,
        "only one coordinate system must be defined\n"));
    }
  } else {
    struct app_model_instance* instance = NULL;
    const char* name = EDIT_CMD_ARGVAL(argv, INSTANCE_NAME).data.string;

    APP(get_model_instance(app, name, &instance));
    if(instance == NULL) {
      APP(log(app, APP_LOG_ERROR, "the instance `%s' does not exist\n", name));
    } else {
      const float scale[3] = {
        [0] = EDIT_CMD_ARGVAL(argv, SCALE_X).is_defined
            ? EDIT_CMD_ARGVAL(argv, SCALE_X).data.real
            : 1.f,
        [1] = EDIT_CMD_ARGVAL(argv, SCALE_Y).is_defined
            ? EDIT_CMD_ARGVAL(argv, SCALE_Y).data.real
            : 1.f,
        [2] = EDIT_CMD_ARGVAL(argv, SCALE_Z).is_defined
            ? EDIT_CMD_ARGVAL(argv, SCALE_Z).data.real
            : 1.f
      };

      if((scale[0] != 0.f) | (scale[1] != 0.f) | (scale[2] != 0.f)) {
        if(EDIT_CMD_ARGVAL(argv, LOCAL_SPACE_FLAG).is_defined) {
          APP(scale_model_instances(&instance, 1, true, scale));
        } else if(EDIT_CMD_ARGVAL(argv, WORLD_SPACE_FLAG).is_defined) {
          APP(scale_model_instances(&instance, 1, false, scale));
        } else {
          struct aosf44 inv_view_4x4;
          struct aosf44 tmp_4x4;
          struct aosf33 scale_3x3;
          struct aosf33 tmp_3x3;
          vf4_t vec;
          UNUSED ALIGN(16) float tmp[4];
          const struct aosf44* view_4x4 = NULL;
          struct app_view* view = NULL;
          assert(EDIT_CMD_ARGVAL(argv, EYE_SPACE_FLAG).is_defined);

          /* Get the view matrix and its inverse. */
          APP(get_main_view(app, &view));
          APP(get_raw_view_transform(view, &view_4x4));
          vec = aosf44_inverse(&inv_view_4x4, view_4x4);
          assert((vf4_store(tmp, vec), tmp[0] != 0.f));

          /* Compute the matrix to apply to the model. */
          scale_3x3.c0 = vf4_set(scale[0], 0.f, 0.f, 0.f);
          scale_3x3.c1 = vf4_set(0.f, scale[1], 0.f, 0.f);
          scale_3x3.c2 = vf4_set(0.f, 0.f, scale[2], 0.f);
          aosf33_set(&tmp_3x3, view_4x4->c0, view_4x4->c1, view_4x4->c2);
          aosf33_mulf33(&tmp_3x3, &scale_3x3, &tmp_3x3);
          aosf44_set(&tmp_4x4, tmp_3x3.c0, tmp_3x3.c1, tmp_3x3.c2,view_4x4->c3);
          aosf44_mulf44(&tmp_4x4, &inv_view_4x4, &tmp_4x4);

          APP(transform_model_instances(&instance, 1, true, &tmp_4x4));
        }
      }
    }
  }
}

static void
transform
  (struct app* app,
   size_t argc UNUSED,
   const struct app_cmdarg** argv,
   void* data UNUSED)
{
  struct app_model_instance* instance = NULL;
  const char* name = NULL;
  enum {
    CMD_NAME, INSTANCE_NAME,
    C0_X, C0_Y, C0_Z, C0_W,
    C1_X, C1_Y, C1_Z, C1_W,
    C2_X, C2_Y, C2_Z, C2_W,
    C3_X, C3_Y, C3_Z, C3_W,
    ARGC
  };
  assert(app != NULL
      && argc == ARGC
      && argv != NULL
      && argv[CMD_NAME]->type == APP_CMDARG_STRING
      && argv[INSTANCE_NAME]->type == APP_CMDARG_STRING
      && argv[C0_X]->type == APP_CMDARG_FLOAT
      && argv[C0_Y]->type == APP_CMDARG_FLOAT
      && argv[C0_Z]->type == APP_CMDARG_FLOAT
      && argv[C0_W]->type == APP_CMDARG_FLOAT
      && argv[C1_X]->type == APP_CMDARG_FLOAT
      && argv[C1_Y]->type == APP_CMDARG_FLOAT
      && argv[C1_Z]->type == APP_CMDARG_FLOAT
      && argv[C1_W]->type == APP_CMDARG_FLOAT
      && argv[C2_X]->type == APP_CMDARG_FLOAT
      && argv[C2_Y]->type == APP_CMDARG_FLOAT
      && argv[C2_Z]->type == APP_CMDARG_FLOAT
      && argv[C2_W]->type == APP_CMDARG_FLOAT
      && argv[C3_X]->type == APP_CMDARG_FLOAT
      && argv[C3_Y]->type == APP_CMDARG_FLOAT
      && argv[C3_Z]->type == APP_CMDARG_FLOAT
      && argv[C3_W]->type == APP_CMDARG_FLOAT);

  name = EDIT_CMD_ARGVAL(argv, INSTANCE_NAME).data.string;
  APP(get_model_instance(app, name, &instance));
  if(instance == NULL) {
    APP(log(app, APP_LOG_ERROR, "the instance `%s' does not exist\n", name));
  } else {
    const struct aosf44 f44 = {
      .c0 = vf4_set
        (EDIT_CMD_ARGVAL(argv, C0_X).data.real,
         EDIT_CMD_ARGVAL(argv, C0_Y).data.real,
         EDIT_CMD_ARGVAL(argv, C0_Z).data.real,
         EDIT_CMD_ARGVAL(argv, C0_W).data.real),
      .c1 = vf4_set
        (EDIT_CMD_ARGVAL(argv, C1_X).data.real,
         EDIT_CMD_ARGVAL(argv, C1_Y).data.real,
         EDIT_CMD_ARGVAL(argv, C1_Z).data.real,
         EDIT_CMD_ARGVAL(argv, C1_W).data.real),
      .c2 = vf4_set
        (EDIT_CMD_ARGVAL(argv, C2_X).data.real,
         EDIT_CMD_ARGVAL(argv, C2_Y).data.real,
         EDIT_CMD_ARGVAL(argv, C2_Z).data.real,
         EDIT_CMD_ARGVAL(argv, C2_W).data.real),
      .c3 = vf4_set
        (EDIT_CMD_ARGVAL(argv, C3_X).data.real,
         EDIT_CMD_ARGVAL(argv, C3_Y).data.real,
         EDIT_CMD_ARGVAL(argv, C3_Z).data.real,
         EDIT_CMD_ARGVAL(argv, C3_W).data.real)
    };
    APP(transform_model_instances(&instance, 1, false, &f44));
  }
}

/*******************************************************************************
 *
 * Move command registration functions.
 *
 ******************************************************************************/
enum edit_error
edit_release_move_commands(struct edit_context* ctxt)
{
  if(UNLIKELY(!ctxt))
    return EDIT_INVALID_ARGUMENT;

  #define RELEASE_CMD(cmd) \
    do { \
      bool b = false; \
      APP(has_command(ctxt->app, cmd, &b)); \
      if(b == true) \
        APP(del_command(ctxt->app, cmd)); \
    } while(0);

  RELEASE_CMD("mv");
  RELEASE_CMD("rotate");
  RELEASE_CMD("scale");
  RELEASE_CMD("transform");
  RELEASE_CMD("translate");

  #undef RELEASE_CMD
  return EDIT_NO_ERROR;
}

enum edit_error
edit_setup_move_commands(struct edit_context* ctxt)
{
  enum edit_error edit_err = EDIT_NO_ERROR;
  if(UNLIKELY(!ctxt)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }

  #define CALL(func) \
    do { \
      const enum app_error app_err = func; \
      if(APP_NO_ERROR != app_err) { \
        edit_err = app_to_edit_error(app_err); \
        goto error; \
      } \
    } while(0)

  CALL(app_add_command
    (ctxt->app, "mv", mv, NULL, app_model_instance_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING
        ("i", "instance", "<name>", "define the instance to move", 1, 1, NULL),
      APP_CMDARG_APPEND_FLOAT
        ("x", NULL, "<real>", NULL, 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_FLOAT
        ("y", NULL, "<real>", NULL, 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_FLOAT
        ("z", NULL, "<real>", NULL, 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_END),
     "move a model instance"));
  CALL(app_add_command
    (ctxt->app, "mv", mv_selection, ctxt, NULL,
     APP_CMDARGV
     (APP_CMDARG_APPEND_LITERAL("s", "selection", NULL, 1, 1),
      APP_CMDARG_APPEND_FLOAT
        ("x", NULL, "<real>", NULL, 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_FLOAT
        ("y", NULL, "<real>", NULL, 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_FLOAT
        ("z", NULL, "<real>", NULL, 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_END),
     "move the selected instances"));

  CALL(app_add_command
    (ctxt->app, "rotate", rotate, NULL, app_model_instance_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_LITERAL
        ("e", "eye",
         "perform the rotation with respect to eye coordinates",
         0, 1),
      APP_CMDARG_APPEND_LITERAL
        ("l", "local",
         "perform the rotation with respect to object coordinates",
         0, 1),
      APP_CMDARG_APPEND_LITERAL
        ("w", "world",
         "perform the rotation with respect to world coordinates",
         0, 1),
      APP_CMDARG_APPEND_STRING
        ("i", "instance", "<name>", "define the instance to rotate", 1, 1,NULL),
      APP_CMDARG_APPEND_FLOAT
        ("x", NULL, "<real>", "pitch angle", 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_FLOAT
        ("y", NULL, "<real>", "yaw angle", 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_FLOAT
        ("z", NULL, "<real>", "roll angle", 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_LITERAL
        ("r", "radian", "set the angle unit to radian", 0, 1),
      APP_CMDARG_END),
     "rotate a model instance"));

  CALL(app_add_command
    (ctxt->app, "scale", scale, NULL, app_model_instance_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_LITERAL
        ("e", "eye",
         "perform the scale with respect to eye coordinates",
         0, 1),
      APP_CMDARG_APPEND_LITERAL
        ("l", "local",
         "perform the scale with respect to object coordinates",
         0, 1),
      APP_CMDARG_APPEND_LITERAL
        ("w", "world",
         "perform the scale with respect to world coordinates",
         0, 1),
      APP_CMDARG_APPEND_STRING
        ("i", "instance", "<name>", "define the instance to scale", 1, 1, NULL),
      APP_CMDARG_APPEND_FLOAT
        ("x", NULL, "<real>", NULL, 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_FLOAT
        ("y", NULL, "<real>", NULL, 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_APPEND_FLOAT
        ("z", NULL, "<real>", NULL, 0, 1, -FLT_MAX, FLT_MAX),
      APP_CMDARG_END),
     "scale a model instance"));

  CALL(app_add_command
    (ctxt->app, "transform", transform, NULL, app_model_instance_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_STRING
        ("i", "instance", "<name>",
         "define the instance to transform",
         1, 1, NULL),
      APP_CMDARG_APPEND_FLOAT(NULL,"e00","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e10","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e20","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e30","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e01","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e11","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e21","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e31","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e02","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e12","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e22","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e32","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e03","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e13","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e23","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_APPEND_FLOAT(NULL,"e33","<real>",NULL,1,1,-FLT_MAX,FLT_MAX),
      APP_CMDARG_END),
     "transform the instance by a 4x4 matrix"));

  CALL(app_add_command
    (ctxt->app, "translate", translate, NULL, app_model_instance_name_completion,
     APP_CMDARGV
     (APP_CMDARG_APPEND_LITERAL
        ("e", "eye",
         "perform the translation with respect to eye coordinates",
         0, 1),
      APP_CMDARG_APPEND_LITERAL
        ("l", "local",
         "perform the translation with respect to object coordinates",
         0, 1),
      APP_CMDARG_APPEND_LITERAL
        ("w", "world",
         "perform the translation with respect to world coordinates",
         0, 1),
      APP_CMDARG_APPEND_STRING
        ("i", "instance", "<name>",
         "define the instance to translate",
         1, 1, NULL),
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
  return edit_err;
error:
  if(ctxt)
    EDIT(release_move_commands(ctxt));
  goto exit;
}

