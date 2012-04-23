#include "app/core/app.h"
#include "app/core/app_command.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "utest/utest.h"
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define BAD_ARG APP_INVALID_ARGUMENT
#define CMD_ERR APP_COMMAND_ERROR
#define OK APP_NO_ERROR

static void
foo(struct app* app UNUSED, size_t argc, const struct app_cmdarg** argv)
{
  CHECK(argc > 0 && argc < 3, true);
  CHECK(argv[0]->type, APP_CMDARG_STRING);
  CHECK(argv[0]->count, 1);
  CHECK(argv[0]->value_list[0].is_defined, true);
  if(argc > 1) {
    CHECK(argv[1]->type, APP_CMDARG_LITERAL);
    CHECK(argv[1]->count, 1);
    CHECK(argv[1]->value_list[0].is_defined, true);
  }
  if(argc > 1) {
    printf("verbose %s\n", argv[0]->value_list[0].data.string);
  } else {
    printf("%s\n", argv[0]->value_list[0].data.string);
  }
  CHECK(strcmp(argv[0]->value_list[0].data.string, "__foo"), 0);
}

static const char* load_name__ = NULL;
static const char* load_model__ = NULL;
static bool load_verbose_opt__ = false;
static bool load_name_opt__ = false;

static void
load(struct app* app UNUSED, size_t argc, const struct app_cmdarg** argv)
{
  CHECK(argc, 4);
  /* Command name. */
  CHECK(argv[0]->type, APP_CMDARG_STRING);
  CHECK(argv[0]->count, 1);
  CHECK(argv[0]->value_list[0].is_defined, true);
  /* Verbose option. */
  CHECK(argv[1]->type, APP_CMDARG_LITERAL);
  CHECK(argv[1]->count, 1);
  CHECK(argv[1]->value_list[0].is_defined, load_verbose_opt__);
  /* Model path option. */
  CHECK(argv[2]->type, APP_CMDARG_FILE);
  CHECK(argv[2]->count, 1);
  CHECK(argv[2]->value_list[0].is_defined, true);
  /* Model name option. */
  CHECK(argv[3]->type, APP_CMDARG_STRING);
  CHECK(argv[3]->count, 1);
  CHECK(argv[3]->value_list[0].is_defined, load_name_opt__);

  CHECK(strcmp(argv[0]->value_list[0].data.string, "__load"), 0);

  if(argv[1]->value_list[0].is_defined)
    printf("verbose ");

  CHECK(strcmp(argv[2]->value_list[0].data.string, load_model__), 0);
  printf("%s MODEL==%s",
         argv[0]->value_list[0].data.string,
         argv[2]->value_list[0].data.string);

  if(argv[3]->value_list[0].is_defined) {
    CHECK(strcmp(argv[3]->value_list[0].data.string, load_name__), 0);
    printf(" NAME=%s\n", argv[3]->value_list[0].data.string);
  } else {
    printf("\n");
  }
}

static bool setf3_r_opt__ = false;
static bool setf3_g_opt__ = false;
static bool setf3_b_opt__ = false;
static float setf3_r__ = 0.f;
static float setf3_g__ = 0.f;
static float setf3_b__ = 0.f;

static void
setf3(struct app* app UNUSED, size_t argc,  const struct app_cmdarg** argv)
{
  CHECK(argc, 4);
  /* Command name. */
  CHECK(argv[0]->type, APP_CMDARG_STRING);
  CHECK(argv[0]->count, 1);
  CHECK(argv[0]->value_list[0].is_defined, true);
  /* Red option. */
  CHECK(argv[1]->type, APP_CMDARG_FLOAT);
  CHECK(argv[1]->count, 1);
  CHECK(argv[1]->value_list[0].is_defined, setf3_r_opt__);
  /* Green option. */
  CHECK(argv[2]->type, APP_CMDARG_FLOAT);
  CHECK(argv[2]->count, 1);
  CHECK(argv[2]->value_list[0].is_defined, setf3_g_opt__);
  /* Blue option. */
  CHECK(argv[3]->type, APP_CMDARG_FLOAT);
  CHECK(argv[3]->count, 1);
  CHECK(argv[3]->value_list[0].is_defined, setf3_b_opt__);

  CHECK(strcmp(argv[0]->value_list[0].data.string, "__setf3"), 0);
  if(argv[1]->value_list[0].is_defined)
    CHECK(argv[1]->value_list[0].data.real, setf3_r__);
  if(argv[2]->value_list[0].is_defined)
    CHECK(argv[2]->value_list[0].data.real, setf3_g__);
  if(argv[3]->value_list[0].is_defined)
    CHECK(argv[3]->value_list[0].data.real, setf3_b__);
}


#define MAX_DAY_COUNT 3
static const char* day_list__[MAX_DAY_COUNT];
static size_t day_count__ = 0;

static const char* days[] = {
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday",
  "Sunday",
  NULL
};

static void
day(struct app* app UNUSED, size_t argc, const struct app_cmdarg** argv)
{
  size_t i = 0;

  CHECK(argc, 2);
  /* Command name. */
  CHECK(argv[0]->type, APP_CMDARG_STRING);
  CHECK(argv[0]->count, 1);
  CHECK(argv[0]->value_list[0].is_defined, true);
  /* Filter name. */
  CHECK(argv[1]->type, APP_CMDARG_STRING);
  CHECK(argv[1]->count, MAX_DAY_COUNT);
  CHECK(argv[1]->value_list[0].is_defined, true);

  CHECK(strcmp(argv[0]->value_list[0].data.string, "__day"), 0);

  for(i = 0; i < argv[1]->count && argv[1]->value_list[i].is_defined; ++i) {
    size_t j = 0;

    CHECK(strcmp(argv[1]->value_list[i].data.string, day_list__[i]), 0);
    for(j = 0; days[j]; ++j) {
      if(strcmp(days[j], argv[1]->value_list[i].data.string) == 0)
        break;
    }
    NCHECK(days[i], NULL);
  }
  CHECK(i, day_count__);
}

#define MAX_FILE_COUNT 4
static const char* cat_file_list__[MAX_FILE_COUNT];
static size_t cat_file_count__ = 0;

static void
cat(struct app* app UNUSED, size_t argc, const struct app_cmdarg** argv)
{
  size_t i = 0;

  CHECK(argc, 2);
  /* Command name. */
  CHECK(argv[0]->type, APP_CMDARG_STRING);
  CHECK(argv[0]->count, 1);
  CHECK(argv[0]->value_list[0].is_defined, true);
  /* Filter name. */
  CHECK(argv[1]->type, APP_CMDARG_FILE);
  CHECK(argv[1]->count, MAX_FILE_COUNT);

  CHECK(strcmp(argv[0]->value_list[0].data.string, "__cat"), 0);
  for(i = 0; i < argv[1]->count && argv[1]->value_list[i].is_defined; ++i) {
    CHECK(strcmp(argv[1]->value_list[i].data.string, cat_file_list__[i]), 0);
  }
  CHECK(i, cat_file_count__);
}

#define MAX_INT_COUNT 3
static int seti_int_list__[MAX_INT_COUNT];
static int seti_int_count__ = 0;


static void
seti(struct app* app UNUSED, size_t argc, const struct app_cmdarg** argv)
{
  int i = 0;

  CHECK(argc, 2);
  /* Command name. */
  CHECK(argv[0]->type, APP_CMDARG_STRING);
  CHECK(argv[0]->count, 1);
  CHECK(argv[0]->value_list[0].is_defined, true);
  /* Filter name. */
  CHECK(argv[1]->type, APP_CMDARG_INT);
  CHECK(argv[1]->count, MAX_INT_COUNT);

  CHECK(strcmp(argv[0]->value_list[0].data.string, "__seti"), 0);
  for(i=0; (size_t)i<argv[1]->count && argv[1]->value_list[i].is_defined; ++i) {
    CHECK(argv[1]->value_list[i].data.integer, seti_int_list__[i]);
  }
  CHECK(i, seti_int_count__);
}

int
main(int argc, char **argv)
{
  struct app_args args = { NULL, NULL, NULL, NULL };
  struct app* app = NULL;
  bool b = false;

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    return -1;
  }
  args.render_driver = argv[1];

  CHECK(app_init(&args, &app), OK);

  CHECK(app_add_command(NULL, NULL, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(app, NULL, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(NULL, NULL, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(app, "__foo", NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(NULL, NULL, foo, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(app, NULL, foo, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(NULL, NULL, foo, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(app, "__foo", foo, NULL, NULL, NULL), OK);
  CHECK(app_add_command(app, "__foo", foo, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_add_command
    (app, "__foo", foo, NULL, APP_CMDARGV(
      APP_CMDARG_APPEND_LITERAL("vV", "verbose, verb", NULL, 0, 1),
      APP_CMDARG_END),
     NULL),
    BAD_ARG);

  CHECK(app_execute_command(NULL, NULL), BAD_ARG);
  CHECK(app_execute_command(app, NULL), BAD_ARG);
  CHECK(app_execute_command(NULL, "__foox"), BAD_ARG);
  CHECK(app_execute_command(app, "__foox"), CMD_ERR);
  CHECK(app_execute_command(app, "__foo"), OK);

  CHECK(app_del_command(NULL, NULL), BAD_ARG);
  CHECK(app_del_command(app, NULL), BAD_ARG);
  CHECK(app_del_command(NULL, "__foox"), BAD_ARG);
  CHECK(app_del_command(app, "__foox"), BAD_ARG);
  CHECK(app_del_command(app, "__foo"), OK);
  CHECK(app_execute_command(app, "__foo"), CMD_ERR);

  CHECK(app_add_command
    (app, "__foo", foo, NULL,
     APP_CMDARGV
      (APP_CMDARG_APPEND_LITERAL("vV", "verb,verbose", NULL, 0, 1),
       APP_CMDARG_END),
      NULL),
    OK);
  CHECK(app_execute_command(app, "__foo -v"), OK);
  CHECK(app_execute_command(app, "__foo --verbose"), OK);
  CHECK(app_execute_command(app, "__foo -verbose"), CMD_ERR);
  CHECK(app_execute_command(app, "__foo -V"), OK);
  CHECK(app_execute_command(app, "__foo --verb"), OK);
  CHECK(app_execute_command(app, "__foo -vV"), CMD_ERR);
  CHECK(app_del_command(app, "__foo"), OK);

  CHECK(app_add_command
    (app, "__load", load, NULL,
     APP_CMDARGV
      (APP_CMDARG_APPEND_LITERAL("vV", "verbose,verb", "verbosity", 0, 1),
       APP_CMDARG_APPEND_FILE("mM", "model,mdl", "<model-path>", NULL, 1, 1),
       APP_CMDARG_APPEND_STRING("n", "name", "<model-name>", NULL, 0, 1, NULL),
       APP_CMDARG_END),
      "load a model"),
    OK);

  load_verbose_opt__ = false;
  load_name_opt__ = false;
  load_model__ = "\"my_model.obj\"";
  CHECK(app_execute_command(app, "__load"), CMD_ERR);
  CHECK(app_execute_command(app, "__load -m \"my_model.obj\""), OK);
  CHECK(app_execute_command(app, "__load -M \"my_model.obj\""), OK);

  load_verbose_opt__ = true;
  CHECK(app_execute_command(app, "__load --verbose -m \"my_model.obj\""), OK);
  CHECK(app_execute_command(app, "__load -vm \"my_model.obj\""), OK);
  CHECK(app_execute_command(app, "__load -m \"my_model.obj\" -V"), OK);
  CHECK(app_execute_command(app, "__load -mv \"my_model.obj\""), CMD_ERR);
  CHECK(app_execute_command(app, "__load -vm \"my_model.obj\""), OK);
  CHECK(app_execute_command(app, "__load -vVm \"my_model.obj\""), CMD_ERR);

  load_name_opt__ = true;
  load_name__ = "\"foo\"";
  CHECK(app_execute_command
    (app, "__load -m \"my_model.obj\" -V --name=\"foo\""), OK);
  load_name__ = "\"HelloWorld\"";
  CHECK(app_execute_command
    (app, "__load --model \"my_model.obj\" --verb -n \"HelloWorld\""), OK);

  load_verbose_opt__ = false;
  CHECK(app_execute_command
    (app, "__load --model \"my_model.obj\" -n "), CMD_ERR);
  load_name__ = "=my_name";
  CHECK(app_execute_command
    (app, "__load -n=my_name --model \"my_model.obj\""), OK);
  CHECK(app_execute_command
    (app, "__load -n=my_name --model \"my_model.obj\" --name my_name2"), CMD_ERR);
  CHECK(app_execute_command
    (app, "__load -n=my_name --model \"my_model.obj\" -m model0"), CMD_ERR);
  CHECK(app_execute_command
    (app, "__load -n=my_name --model \"my_model.obj\""), OK);

  CHECK(app_man_command(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_man_command(app, NULL, NULL), BAD_ARG);
  CHECK(app_man_command(NULL, "_load_", NULL), BAD_ARG);
  CHECK(app_man_command(app, "_load_", NULL), BAD_ARG);
  CHECK(app_man_command(NULL, NULL, stdout), BAD_ARG);
  CHECK(app_man_command(app, NULL, stdout), BAD_ARG);
  CHECK(app_man_command(NULL, "_load_", stdout), BAD_ARG);
  CHECK(app_man_command(app, "_load_", stdout), CMD_ERR);
  CHECK(app_man_command(app, "__load", stdout), OK);

  CHECK(app_add_command
    (app, "__setf3", setf3, NULL,
     APP_CMDARGV
      (APP_CMDARG_APPEND_FLOAT
        ("r", "red", "<real>", "red value", 0, 1, 0.f, 1.f),
       APP_CMDARG_APPEND_FLOAT
        ("g", "green", "<real>", "green value", 0, 1, 0.f, 1.f),
       APP_CMDARG_APPEND_FLOAT
        ("b", "blue", "<real>", "blue value", 0, 1, 0.f, 1.f),
       APP_CMDARG_END
      ),
      NULL),
    OK);

  setf3_r_opt__ = false;
  setf3_g_opt__ = false;
  setf3_b_opt__ = false;
  setf3_r__ = 0.f;
  setf3_g__ = 0.f;
  setf3_b__ = 0.f;
  CHECK(app_execute_command(app, "__setf3"), OK);
  setf3_g_opt__ = true;
  setf3_g__ = 1.0f;
  CHECK(app_execute_command(app, "__setf3 -g 5.1"), OK);
  CHECK(app_execute_command(app, "__setf3 -g 1.0"), OK);
  setf3_g__ = 0.78f;
  CHECK(app_execute_command(app, "__setf3 -g 0.78"), OK);
  setf3_r_opt__ = true;
  setf3_r__ = 0.f;
  CHECK(app_execute_command(app, "__setf3 -g 0.78 -r -1.5"), OK);
  CHECK(app_execute_command(app, "__setf3 --red=-1.5 -g 0.78"), OK);
  setf3_b_opt__ = true;
  setf3_b__ = 0.5f;
  CHECK(app_execute_command(app, "__setf3 --red=-1.5 --blue 0.5 -g 0.78"), OK);
  CHECK(app_execute_command
    (app, "__setf3 -r -1.5 -b 0.5e0 -g 0.78 -g 1"), CMD_ERR);

  CHECK(app_add_command
    (app, "__day", day, NULL,
     APP_CMDARGV
      (APP_CMDARG_APPEND_STRING(NULL, NULL, "<day>", "day name", 1, 3, days),
       APP_CMDARG_END),
      NULL),
    OK);
  day_count__ = 0;
  memset(day_list__, 0, sizeof(day_list__));

  day_count__ = 1;
  day_list__[0] = "foo";
  CHECK(app_execute_command(app, "day"), CMD_ERR);
  CHECK(app_execute_command(app, "__day"), CMD_ERR);
  CHECK(app_execute_command(app, "__day foo"), CMD_ERR);
  day_list__[0] = "Monday";
  CHECK(app_execute_command(app, "__day Monday"), OK);
  day_list__[0] = "Tuesday";
  CHECK(app_execute_command(app, "__day Tuesday"), OK);
  day_list__[0] = "Wednesday";
  CHECK(app_execute_command(app, "__day Wednesday"), OK);
  day_list__[0] = "Thursday";
  CHECK(app_execute_command(app, "__day Thursday"), OK);
  day_list__[0] = "Friday";
  CHECK(app_execute_command(app, "__day Friday"), OK);
  day_list__[0] = "Saturday";
  CHECK(app_execute_command(app, "__day Saturday"), OK);
  day_list__[0] = "Sunday";
  CHECK(app_execute_command(app, "__day Sunday"), OK);
  CHECK(app_execute_command(app, "__day sunday"), CMD_ERR);
  day_count__++;
  day_list__[1] = "foo";
  CHECK(app_execute_command(app, "__day Sunday foo"), CMD_ERR);
  day_list__[0] = "foo0";
  day_list__[1] = "foo1";
  CHECK(app_execute_command(app, "__day foo0 foo1"), CMD_ERR);
  day_list__[0] = "Monday";
  day_list__[1] = "Friday";
  CHECK(app_execute_command(app, "__day Monday Friday"), OK);
  day_list__[day_count__++] = "Saturday";
  CHECK(app_execute_command(app, "__day Monday Friday Saturday"), OK);

  CHECK(app_add_command
    (app, "__cat", cat, NULL,
     APP_CMDARGV
      (APP_CMDARG_APPEND_FILE(NULL, NULL, "<file> ...", "files", 1, MAX_FILE_COUNT),
       APP_CMDARG_END),
      NULL),
    OK);
  cat_file_count__ = 0;
  memset(cat_file_list__, 0, sizeof(cat_file_list__));

  CHECK(app_execute_command(app, "__cat"), CMD_ERR);
  cat_file_list__[cat_file_count__++] = "foo";
  CHECK(app_execute_command(app, "__cat foo"), OK);
  cat_file_list__[cat_file_count__++] = "hello";
  CHECK(app_execute_command(app, "__cat foo hello"), OK);
  cat_file_list__[cat_file_count__++] = "world";
  CHECK(app_execute_command(app, "__cat foo hello world"), OK);
  cat_file_list__[cat_file_count__++] = "test";
  CHECK(app_execute_command(app, "__cat foo hello world test"), OK);

  CHECK(app_add_command
    (app, "__seti", seti, NULL,
     APP_CMDARGV
      (APP_CMDARG_APPEND_INT("-i", NULL, NULL, NULL, 1, MAX_INT_COUNT, 5, 7),
       APP_CMDARG_END),
      "set a list of integer"),
    OK);
  seti_int_count__ = 0;
  memset(seti_int_list__, 0, sizeof(seti_int_list__));

  seti_int_list__[seti_int_count__++] = 5;
  CHECK(app_execute_command(app, "__seti 0"), CMD_ERR);
  CHECK(app_execute_command(app, "__seti -i 0"), OK);
  seti_int_list__[seti_int_count__++] = 5;
  CHECK(app_execute_command(app, "__seti -i 0 -i -8"), OK);
  seti_int_list__[seti_int_count__ - 1] = 6;
  CHECK(app_execute_command(app, "__seti -i 0 -i 6"), OK);
  seti_int_list__[seti_int_count__++] = 7;
  CHECK(app_execute_command(app, "__seti -i 0 -i 6 -i 9"), OK);

  CHECK(app_has_command(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_has_command(app, NULL, NULL), BAD_ARG);
  CHECK(app_has_command(NULL, "__seti", NULL), BAD_ARG);
  CHECK(app_has_command(app, "__seti", NULL), BAD_ARG);
  CHECK(app_has_command(NULL, NULL, &b), BAD_ARG);
  CHECK(app_has_command(app, NULL, &b), BAD_ARG);
  CHECK(app_has_command(NULL, "__seti", &b), BAD_ARG);
  CHECK(app_has_command(app, "__seti", &b), OK);
  CHECK(b, true);

  CHECK(app_del_command(app, "__seti"), OK);
  CHECK(app_has_command(app, "__seti", &b), OK);
  CHECK(b, false);

  CHECK(app_cleanup(app), APP_NO_ERROR);
  CHECK(app_ref_put(app), APP_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
}

