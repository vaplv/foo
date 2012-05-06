#include "app/core/app.h"
#include "app/core/app_cvar.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "utest/utest.h"
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define BAD_ARG APP_INVALID_ARGUMENT
#define OK APP_NO_ERROR

int
main(int argc, char** argv)
{
  struct app_args args = { NULL, NULL, NULL, NULL };
  struct app* app = NULL;
  const struct app_cvar* cvar = NULL;
  char buf[128];

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    return -1;
  }
  args.render_driver = argv[1];

  CHECK(app_init(&args, &app), OK);

  CHECK(app_add_cvar(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_add_cvar(app, NULL, NULL), BAD_ARG);
  CHECK(app_add_cvar(NULL, "foo__", NULL), BAD_ARG);
  CHECK(app_add_cvar(app, "foo__", NULL), BAD_ARG);
  CHECK(app_add_cvar(NULL, NULL, APP_CVAR_BOOL_DESC(true)), BAD_ARG);
  CHECK(app_add_cvar(app, NULL, APP_CVAR_BOOL_DESC(true)), BAD_ARG);
  CHECK(app_add_cvar(NULL, "foo__", APP_CVAR_BOOL_DESC(true)), BAD_ARG);
  CHECK(app_add_cvar(app, "foo__", APP_CVAR_BOOL_DESC(true)), OK);

  CHECK(app_get_cvar(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_get_cvar(app, NULL, NULL), BAD_ARG);
  CHECK(app_get_cvar(NULL, "test__", NULL), BAD_ARG);
  CHECK(app_get_cvar(app, "test__", NULL), BAD_ARG);
  CHECK(app_get_cvar(NULL, NULL, &cvar), BAD_ARG);
  CHECK(app_get_cvar(app, NULL, &cvar), BAD_ARG);
  CHECK(app_get_cvar(NULL, "test__", &cvar), BAD_ARG);
  CHECK(app_get_cvar(app, "test__", &cvar), OK);
  CHECK(cvar, NULL);
  CHECK(app_get_cvar(app, "foo__", &cvar), OK);
  NCHECK(cvar, NULL);
  CHECK(cvar->type, APP_CVAR_BOOL);
  CHECK(cvar->value.boolean, true);

  CHECK(app_set_cvar(NULL, NULL, APP_CVAR_BOOL_VALUE(false)), BAD_ARG);
  CHECK(cvar->type, APP_CVAR_BOOL);
  CHECK(cvar->value.boolean, true);
  CHECK(app_set_cvar(app, NULL, APP_CVAR_BOOL_VALUE(false)), BAD_ARG);
  CHECK(cvar->type, APP_CVAR_BOOL);
  CHECK(cvar->value.boolean, true);
  CHECK(app_set_cvar(NULL, "test__", APP_CVAR_BOOL_VALUE(false)), BAD_ARG);
  CHECK(cvar->type, APP_CVAR_BOOL);
  CHECK(cvar->value.boolean, true);
  CHECK(app_set_cvar(app, "test__", APP_CVAR_BOOL_VALUE(false)), BAD_ARG);
  CHECK(cvar->type, APP_CVAR_BOOL);
  CHECK(cvar->value.boolean, true);
  CHECK(app_set_cvar(app, "foo__", APP_CVAR_BOOL_VALUE(false)), OK);
  CHECK(cvar->type, APP_CVAR_BOOL);
  CHECK(cvar->value.boolean, false);

  CHECK(app_del_cvar(NULL, NULL), BAD_ARG);
  CHECK(app_del_cvar(app, NULL), BAD_ARG);
  CHECK(app_del_cvar(NULL, "test__"), BAD_ARG);
  CHECK(app_del_cvar(app, "test__"), BAD_ARG);
  CHECK(app_del_cvar(app, "foo__"), OK);

  CHECK(app_get_cvar(app, "foo__", &cvar), OK);
  CHECK(cvar, NULL);
  CHECK(app_set_cvar(app, "foo__", APP_CVAR_BOOL_VALUE(false)), BAD_ARG);

  CHECK(app_add_cvar(app, "fooi__", APP_CVAR_INT_DESC(-8, -5, 5)), OK);
  CHECK(app_get_cvar(app, "fooi__", &cvar), OK);
  CHECK(cvar->type, APP_CVAR_INT);
  CHECK(cvar->value.integer, -5);
  CHECK(app_set_cvar(app, "fooi__", APP_CVAR_INT_VALUE(1)), OK);
  CHECK(cvar->type, APP_CVAR_INT);
  CHECK(cvar->value.integer, 1);
  CHECK(app_set_cvar(app, "fooi__", APP_CVAR_INT_VALUE(6)), OK);
  CHECK(cvar->type, APP_CVAR_INT);
  CHECK(cvar->value.integer, 5);

  CHECK(app_add_cvar
    (app, "fooi__", APP_CVAR_FLOAT_DESC(1.5f, -1.7f, 5.f)), BAD_ARG);
  CHECK(app_add_cvar(app, "foof__", APP_CVAR_FLOAT_DESC(1.5f, -1.7f, 5.f)), OK);
  CHECK(app_get_cvar(app, "foof__", &cvar), OK);
  CHECK(cvar->type, APP_CVAR_FLOAT);
  CHECK(cvar->value.real, 1.5f);
  CHECK(app_set_cvar(app, "foof__", APP_CVAR_FLOAT_VALUE(6.5f)), OK);
  CHECK(cvar->type, APP_CVAR_FLOAT);
  CHECK(cvar->value.real, 5.0f);
  CHECK(app_set_cvar(app, "foof__", APP_CVAR_FLOAT_VALUE(-1.75f)), OK);
  CHECK(cvar->type, APP_CVAR_FLOAT);
  CHECK(cvar->type, APP_CVAR_FLOAT);
  CHECK(cvar->value.real, -1.7f);
  CHECK(app_get_cvar(app, "fooi__", &cvar), OK);
  CHECK(cvar->type, APP_CVAR_INT);
  CHECK(cvar->value.integer, 5);

  CHECK(app_add_cvar(app, "foo__", APP_CVAR_STRING_DESC(NULL, NULL)), OK);
  CHECK(app_get_cvar(app, "foo__", &cvar), OK);
  CHECK(cvar->type, APP_CVAR_STRING);
  CHECK(cvar->value.string, NULL);
  CHECK(app_set_cvar(app, "foo__", APP_CVAR_STRING_VALUE("foo")), OK);
  CHECK(cvar->type, APP_CVAR_STRING);
  CHECK(strcmp(cvar->value.string, "foo"), 0);
  strcpy(buf, "test");
  CHECK(app_set_cvar(app, "foo__", APP_CVAR_STRING_VALUE(buf)), OK);
  CHECK(cvar->type, APP_CVAR_STRING);
  CHECK(strcmp(cvar->value.string, "test"), 0);
  /* Ensure that the cvar does not directly point buf .*/
  buf[0] = '\0';
  CHECK(cvar->type, APP_CVAR_STRING);
  CHECK(strcmp(cvar->value.string, "test"), 0);
  CHECK(app_del_cvar(app, "foo__"), OK);

  strcpy(buf, "hello");
  CHECK(app_add_cvar
    (app, "foo__",
     APP_CVAR_STRING_DESC(buf, ((const char*[]){"hello",  "world", NULL}))),
    OK);
  CHECK(app_get_cvar(app, "foo__", &cvar), OK);
  CHECK(cvar->type, APP_CVAR_STRING);
  NCHECK(cvar->value.string, NULL);
  CHECK(strcmp(cvar->value.string, "hello"), 0);
  buf[0] = '\0';
  CHECK(strcmp(cvar->value.string, "hello"), 0);
  CHECK(app_set_cvar(app, "foo__", APP_CVAR_STRING_VALUE("world!")), OK);
  CHECK(strcmp(cvar->value.string, "hello"), 0);
  CHECK(app_set_cvar(app, "foo__", APP_CVAR_STRING_VALUE("world")), OK);
  CHECK(strcmp(cvar->value.string, "world"), 0);

  CHECK(app_add_cvar
    (app, "foo1__",
     APP_CVAR_STRING_DESC("hop", ((const char*[]){"hello",  "world", NULL}))),
    OK);
  CHECK(app_get_cvar(app, "foo1__", &cvar), OK);
  CHECK(cvar->type, APP_CVAR_STRING);
  CHECK(cvar->value.string, "hello");
  CHECK(app_get_cvar(app, "foo__", &cvar), OK);
  CHECK(cvar->type, APP_CVAR_STRING);
  CHECK(cvar->value.string, "world");
  CHECK(app_add_cvar
    (app, "foo2__",
     APP_CVAR_STRING_DESC(NULL, ((const char*[]){"hello",  "world", NULL}))),
    OK);
  CHECK(app_get_cvar(app, "foo2__", &cvar), OK);
  CHECK(cvar->type, APP_CVAR_STRING);
  CHECK(cvar->value.string, "hello");
  CHECK(app_get_cvar(app, "foo1__", &cvar), OK);
  CHECK(cvar->type, APP_CVAR_STRING);
  CHECK(cvar->value.string, "hello");
  CHECK(app_get_cvar(app, "foo__", &cvar), OK);
  CHECK(cvar->type, APP_CVAR_STRING);
  CHECK(cvar->value.string, "world");
  CHECK(app_get_cvar(app, "foof__", &cvar), OK);
  CHECK(cvar->type, APP_CVAR_FLOAT);
  CHECK(cvar->value.real, -1.7f);
  CHECK(app_get_cvar(app, "fooi__", &cvar), OK);
  CHECK(cvar->type, APP_CVAR_INT);
  CHECK(cvar->value.integer, 5);

  CHECK(app_cleanup(app), APP_NO_ERROR);
  CHECK(app_ref_put(app), APP_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);
  return 0;
}
