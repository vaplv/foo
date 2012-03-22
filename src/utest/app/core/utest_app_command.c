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
#define OK APP_NO_ERROR

#define I -5
#define F0 -100.f
#define F1 100.f
#define F2 3.14159f
#define MIN_FBOUND -1.f
#define MAX_FBOUND 4.f
#define MIN_IBOUND 0
#define MAX_IBOUND 10

static const char* day_list[] = {
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
foo(struct app* app UNUSED, size_t argc, struct app_cmdarg* argv UNUSED)
{
  CHECK(argc, 1);
  CHECK(argv[0].type, APP_CMDARG_STRING);
  printf("%s\n", argv[0].value.string);
  CHECK(strcmp(argv[0].value.string, "foo"), 0);
}

static void
setf3(struct app* app UNUSED, size_t argc, struct app_cmdarg* argv)
{
  CHECK(argc, 4);
  NCHECK(argv, NULL);
  CHECK(argv[0].type, APP_CMDARG_STRING);
  CHECK(argv[1].type, APP_CMDARG_FLOAT);
  CHECK(argv[2].type, APP_CMDARG_FLOAT);
  CHECK(argv[3].type, APP_CMDARG_FLOAT);
  printf
    ("%s %f %f %f\n", 
     argv[0].value.string,
     argv[1].value.real,
     argv[2].value.real,
     argv[3].value.real);
  CHECK(strcmp(argv[0].value.string, "setf3"), 0);
  CHECK(argv[1].value.real, F0);
  CHECK(argv[2].value.real, F1);
  CHECK(argv[3].value.real, F2);
}

static void
setf3_clamp(struct app* app UNUSED, size_t argc, struct app_cmdarg* argv)
{
  CHECK(argc, 4);
  NCHECK(argv, NULL);
  CHECK(argv[0].type, APP_CMDARG_STRING);
  CHECK(argv[1].type, APP_CMDARG_FLOAT);
  CHECK(argv[2].type, APP_CMDARG_FLOAT);
  CHECK(argv[3].type, APP_CMDARG_FLOAT);
  printf
    ("%s %f %f %f\n", 
     argv[0].value.string,
     argv[1].value.real,
     argv[2].value.real,
     argv[3].value.real);
  CHECK(strcmp(argv[0].value.string, "setf3_clamp"), 0);
  NCHECK(argv[1].value.real, F0);
  NCHECK(argv[2].value.real, F1);
  CHECK(argv[3].value.real, F2);
  CHECK(argv[1].value.real, MIN(MAX(F0, MIN_FBOUND), MAX_FBOUND));
  CHECK(argv[2].value.real, MIN(MAX(F1, MIN_FBOUND), MAX_FBOUND));
  CHECK(argv[3].value.real, MIN(MAX(F2, MIN_FBOUND), MAX_FBOUND));
}

static void
seti(struct app* app UNUSED, size_t argc, struct app_cmdarg* argv)
{
  CHECK(argc, 2);
  CHECK(argv[0].type, APP_CMDARG_STRING);
  CHECK(argv[1].type, APP_CMDARG_INT);
  printf
    ("%s %i\n", 
     argv[0].value.string,
     argv[1].value.integer);
  CHECK(strcmp(argv[0].value.string, "seti"), 0);
  CHECK(argv[1].value.integer, I);
}

static void
seti_clamp(struct app* app UNUSED, size_t argc, struct app_cmdarg* argv)
{
  CHECK(argc, 2);
  CHECK(argv[0].type, APP_CMDARG_STRING);
  CHECK(argv[1].type, APP_CMDARG_INT);
  printf
    ("%s %i\n", 
     argv[0].value.string,
     argv[1].value.integer);
  CHECK(strcmp(argv[0].value.string, "seti_clamp"), 0);
  NCHECK(argv[1].value.integer, I);
  CHECK(argv[1].value.integer, MIN(MAX(I, MIN_IBOUND), MAX_IBOUND));
}

static void
print(struct app* app UNUSED, size_t argc, struct app_cmdarg* argv)
{
  CHECK(argc, 2);
  CHECK(argv[0].type, APP_CMDARG_STRING);
  CHECK(argv[1].type, APP_CMDARG_STRING);
  printf
    ("%s %s\n", 
     argv[0].value.string,
     argv[1].value.string);
  CHECK(strcmp(argv[0].value.string, "print"), 0);
}

int
main(int argc, char** argv)
{
  struct app_args args = { NULL, NULL, NULL, NULL };
  struct app* app = NULL;

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    return -1;
  }
  args.render_driver = argv[1];

  CHECK(app_init(&args, &app), OK);

  CHECK(app_add_command(NULL, NULL, NULL, 0, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(app, NULL, NULL, 0, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(NULL, NULL, NULL, 0, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(app, "foo", NULL, 0, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(NULL, NULL, foo, 0, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(app, NULL, foo, 0, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(NULL, NULL, foo, 0, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(app, "foo", foo, 0, NULL, NULL), OK);
  CHECK(app_add_command(app, "foo", foo, 0, NULL, NULL), BAD_ARG);
  CHECK(app_add_command(app, "foo", foo, 1, APP_CMDARGV
    (APP_CMDARG_APPEND_INT(INT_MAX, INT_MIN)), NULL), BAD_ARG);

  CHECK(app_execute_command(NULL, NULL), BAD_ARG);
  CHECK(app_execute_command(app, NULL), BAD_ARG);
  CHECK(app_execute_command(NULL, "foox"), BAD_ARG);
  CHECK(app_execute_command(app, "foox"), BAD_ARG);
  CHECK(app_execute_command(app, "foo"), OK);

  CHECK(app_del_command(NULL, NULL), BAD_ARG);
  CHECK(app_del_command(app, NULL), BAD_ARG);
  CHECK(app_del_command(NULL, "foox"), BAD_ARG);
  CHECK(app_del_command(app, "foox"), BAD_ARG);
  CHECK(app_del_command(app, "foo"), OK);
  CHECK(app_execute_command(app, "foo"), BAD_ARG);

  CHECK(app_add_command(app, "setf3", setf3, 3, APP_CMDARGV
    (APP_CMDARG_APPEND_FLOAT3(FLT_MAX, -FLT_MAX)), "Usage:"), OK);
  CHECK(app_execute_command(app, "setf3 "STR(F0)" "STR(F1)" "STR(F2)), OK);

  CHECK(app_add_command(app, "setf3_clamp", setf3_clamp, 3, APP_CMDARGV
    (APP_CMDARG_APPEND_FLOAT3(MIN_FBOUND, MAX_FBOUND)), NULL), OK);
  CHECK(app_execute_command(app, "setf3 "STR(F0)" "STR(F1)" "STR(F2)), OK);
  CHECK(app_execute_command(app, "setf3_clamp "STR(F0)" "STR(F1)" "STR(F2)), OK);

  CHECK(app_add_command(app, "seti", seti, 1, APP_CMDARGV
    (APP_CMDARG_APPEND_INT(INT_MAX, INT_MIN)), "None"), OK);
  CHECK(app_execute_command(app, "seti "STR(I)), OK);

  CHECK(app_add_command(app, "seti_clamp", seti_clamp, 1, APP_CMDARGV
    (APP_CMDARG_APPEND_INT(MIN_IBOUND, MAX_IBOUND)), "Foo"), OK);
  CHECK(app_execute_command(app, "seti_clamp "STR(I)), OK);
  CHECK(app_execute_command(app, "seti_clamp "STR(I)" 5 6 8 9"), OK);

  CHECK(app_add_command(app, "print", print, 1, APP_CMDARGV
    (APP_CMDARG_APPEND_STRING(NULL)), NULL), OK);
  CHECK(app_execute_command(app, "print \"Hello world!\""), OK);
  CHECK(app_del_command(app, "print"), OK);
  CHECK(app_add_command(app, "print", print, 1, APP_CMDARGV
    (APP_CMDARG_APPEND_STRING(day_list)), NULL), OK);
  CHECK(app_execute_command(app, "print \"Hello_world!\""), BAD_ARG);
  CHECK(app_execute_command(app, "print Monday"), OK);
  CHECK(app_execute_command(app, "print Tuesday"), OK);
  CHECK(app_execute_command(app, "print Wednesday"), OK);
  CHECK(app_execute_command(app, "print Thursday"), OK);
  CHECK(app_execute_command(app, "print Friday"), OK);
  CHECK(app_execute_command(app, "print Saturday"), OK);
  CHECK(app_execute_command(app, "print Sunday"), OK);
  CHECK(app_execute_command(app, "print saturday"), BAD_ARG);

  CHECK(app_add_command(app, "Error", seti, 1, (struct app_cmdarg_desc[])
    {{.type=APP_NB_CMDARG_TYPES, .domain={.integer={.min=0, .max=0}}}}, NULL),
    BAD_ARG);

  /* Builtin commands. */
  CHECK(app_execute_command(app, "help ls"), OK);
  CHECK(app_execute_command(app, "help load"), OK);
  CHECK(app_execute_command(app, "help help"), OK);
  CHECK(app_execute_command(app, "help exit"), OK);
  CHECK(app_execute_command(app, "help bad_command"), OK);
  CHECK(app_execute_command(app, "load --model bad_path"), OK);
  CHECK(app_execute_command(app, "ls"), BAD_ARG);
  CHECK(app_execute_command(app, "ls --commands"), OK);
  CHECK(app_execute_command(app, "ls --bad_option"), BAD_ARG);
  CHECK(app_execute_command(app, "exit"), OK);

  CHECK(app_ref_put(app), APP_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}

