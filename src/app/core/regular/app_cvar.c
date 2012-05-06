#include "app/core/regular/app_c.h"
#include "app/core/regular/app_cvar_c.h"
#include "app/core/regular/app_error_c.h"
#include "app/core/app_cvar.h"
#include "stdlib/sl_flat_map.h"
#include "stdlib/sl_pair.h"
#include "stdlib/sl_string.h"
#include <assert.h>
#include <string.h>

struct cvar {
  struct app_cvar var;
  struct sl_string* str; /* For string cvar with no pre-defined value list. */
  union app_cvar_domain domain;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static int
cmp_str(const void* a, const void* b)
{
  const char* str0 = *(const char**)a;
  const char* str1 = *(const char**)b;
  return strcmp(str0, str1);
}

static enum app_error
set_cvar_value
  (struct sl_logger* logger,
   const char* name,
   struct cvar* cvar,
   union app_cvar_value val)
{
  enum app_error app_err = APP_NO_ERROR;
  assert(logger && name && cvar);

  switch(cvar->var.type) {
    case APP_CVAR_BOOL:
      cvar->var.value.boolean = val.boolean;
      break;
    case APP_CVAR_INT:
      cvar->var.value.integer = /* Clamp the value to the cvar domain . */
        MAX(MIN(val.integer,cvar->domain.integer.max),cvar->domain.integer.min);
      break;
    case APP_CVAR_FLOAT:
      cvar->var.value.real = /* Clamp the value to the cvar domain . */
        MAX(MIN(val.real, cvar->domain.real.max), cvar->domain.real.min);
      break;
    case APP_CVAR_STRING:
      /* The cvar has not predefined value list => copy the val string  into the
       * cvar string. */
      if(cvar->str != NULL) {
        if(val.string == NULL) {
          SL(clear_string(cvar->str));
          cvar->var.value.string = NULL;
        } else {
          const enum sl_error sl_err = sl_string_set(cvar->str, val.string);
          if(sl_err != SL_NO_ERROR) {
            app_err = APP_MEMORY_ERROR;
            goto error;
          }
          SL(string_get(cvar->str, &cvar->var.value.string));
        }
      /* The cvar *has* predefined value list => the cvar string points to a
       * predefined value. */
      } else {
        size_t i = 0;
        bool unexpected_value = false;

        if(val.string == NULL) {
          unexpected_value = true;
        } else {
          for(i = 0; cvar->domain.string.value_list[i] != NULL; ++i) {
            if(strcmp(cvar->domain.string.value_list[i], val.string) == 0) {
              cvar->var.value.string = cvar->domain.string.value_list[i];
              break;
            }
          }
          unexpected_value = (cvar->domain.string.value_list[i] == NULL);
        }
        if(unexpected_value) {
          APP_LOG_ERR
            (logger,
             "%s: unexpected value `%s'\n",
             name, val.string ? val.string : "<null>");
          if(cvar->var.value.string == NULL) {
            cvar->var.value.string = cvar->domain.string.value_list[0];
            APP_LOG_WARN
              (logger,
               "%s: set default value `%s'\n",
               name, cvar->var.value.string);
          }
        }
      }
      break;
    default: assert(0); break;
  }
exit:
  return app_err;
error:
  goto exit;
}

/*******************************************************************************
 *
 * cvar functions.
 *
 ******************************************************************************/
EXPORT_SYM enum app_error
app_add_cvar
  (struct app* app,
   const char* cvar_name,
   const struct app_cvar_desc* desc)
{
  struct cvar cvar;
  char* name = NULL;
  size_t len = 0;
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool is_added = false;
  memset(&cvar, 0, sizeof(cvar));

  if(!app || !cvar_name || !desc) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  len = strlen(cvar_name) + 1;
  name = MEM_CALLOC(app->allocator, len, sizeof(char));
  if(!name) {
    app_err = APP_MEMORY_ERROR;
    goto error;
  }
  name = memcpy(name, cvar_name, len);

  if(desc->type == APP_CVAR_STRING
  && (  desc->domain.string.value_list == NULL
     || desc->domain.string.value_list[0] == NULL)) {
    sl_err = sl_create_string(NULL, app->allocator, &cvar.str);
    if(sl_err != SL_NO_ERROR) {
      app_err = sl_to_app_error(sl_err);
      goto error;
    }
  } else {
    cvar.domain = desc->domain;
  }
  cvar.var.type = desc->type;
  app_err = set_cvar_value(app->logger, cvar_name, &cvar, desc->init_value);
  if(app_err != APP_NO_ERROR)
    goto error;

  sl_err = sl_flat_map_insert(app->cvar_system.map, &name, &cvar, NULL);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
  is_added = true;
exit:
  return app_err;
error:
  if(is_added)
    SL(flat_map_erase(app->cvar_system.map, (const void*)&cvar_name, NULL));
  if(name)
    MEM_FREE(app->allocator, name);
  if(cvar.str)
    SL(free_string(cvar.str));
  goto exit;
}

EXPORT_SYM enum app_error
app_del_cvar(struct app* app, const char* name)
{
  struct sl_pair pair = { NULL, NULL };

  if(!app || !name)
    return APP_INVALID_ARGUMENT;

  SL(flat_map_find_pair(app->cvar_system.map, (const void*)&name, &pair));
  if(SL_IS_PAIR_VALID(&pair) == false) {
    return APP_INVALID_ARGUMENT;
  } else {
    struct sl_string* str = ((struct cvar*)pair.data)->str;
    char* cvar_name = *(char**)pair.key;

    SL(flat_map_erase(app->cvar_system.map, (const void*)&name, NULL));
    MEM_FREE(app->allocator, cvar_name);
    if(str)
      SL(free_string(str));
  }
  return APP_NO_ERROR;
}

EXPORT_SYM enum app_error
app_set_cvar(struct app* app, const char* name, union app_cvar_value val)
{
  struct cvar* cvar = NULL;
  enum app_error app_err = APP_NO_ERROR;

  if(!app || !name)
    return APP_INVALID_ARGUMENT;

  SL(flat_map_find(app->cvar_system.map, (const void*)&name, (void**)&cvar));
  if(cvar == NULL) {
    APP_LOG_ERR(app->logger, "%s: cvar not found\n", name);
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  app_err = set_cvar_value(app->logger, name, cvar, val);
  if(app_err != APP_NO_ERROR)
    goto error;
exit:
  return app_err;
error:
  goto exit;
}

EXPORT_SYM enum app_error
app_get_cvar
  (struct app* app,
   const char* name,
   const struct app_cvar** out_cvar)
{
  struct cvar* cvar = NULL;

  if(!app || !name || !out_cvar)
    return APP_INVALID_ARGUMENT;

  SL(flat_map_find(app->cvar_system.map, (const void*)&name, (void**)&cvar));
  if(cvar == NULL) {
    *out_cvar = NULL;
  } else {
    *out_cvar = &cvar->var;
  }
  return APP_NO_ERROR;
}

/*******************************************************************************
 *
 * Private function.
 *
 ******************************************************************************/
enum app_error
app_init_cvar_system(struct app* app)
{
  enum app_error app_err = APP_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!app) {
    app_err = APP_INVALID_ARGUMENT;
    goto error;
  }
  sl_err = sl_create_flat_map
    (sizeof(const char*),
     ALIGNOF(const char*),
     sizeof(struct cvar),
     ALIGNOF(struct cvar),
     cmp_str,
     app->allocator,
     &app->cvar_system.map);
  if(sl_err != SL_NO_ERROR) {
    app_err = sl_to_app_error(sl_err);
    goto error;
  }
exit:
  return app_err;
error:
  goto exit;
}

enum app_error
app_shutdown_cvar_system(struct app* app)
{
  char** name_list = NULL;
  struct cvar* cvar_list = NULL;
  size_t len = 0;
  size_t i = 0;

  if(!app)
    return APP_INVALID_ARGUMENT;

  SL(flat_map_key_buffer
    (app->cvar_system.map, &len, NULL, NULL, (void**)&name_list));
  SL(flat_map_data_buffer
    (app->cvar_system.map, &len, NULL, NULL, (void**)&cvar_list));
  for(i = 0; i < len; ++i) {
    MEM_FREE(app->allocator, name_list[i]);
    if(cvar_list[i].str)
      SL(free_string(cvar_list[i].str));
  }
  SL(free_flat_map(app->cvar_system.map));
  return APP_NO_ERROR;
}

