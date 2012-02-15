#ifdef __GNUC__
  /* The strtok_r functon is available from the POSIX.1 standard. */
  #define _POSIX_SOURCE
#endif

#include "resources/regular/rsrc_context_c.h"
#include "resources/regular/rsrc_error_c.h"
#include "resources/regular/rsrc_wavefront_obj_c.h"
#include "resources/rsrc.h"
#include "resources/rsrc_context.h"
#include "resources/rsrc_wavefront_obj.h"
#include "stdlib/sl.h"
#include "stdlib/sl_vector.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct rsrc_wavefront_obj_line line_t;
typedef struct rsrc_wavefront_obj_face face_t;
typedef struct rsrc_wavefront_obj_range range_t;
typedef struct rsrc_wavefront_obj_group group_t;
typedef struct rsrc_wavefront_obj_smooth_group smooth_group_t;
typedef struct rsrc_wavefront_obj_mtl mtl_t;

/*******************************************************************************
 *
 * Minimal lexer.
 *
 ******************************************************************************/
struct lex {
  const char* delimiters;
  char* saveptr;
  char* string;
};

/* Consume the next token of the lexer.
 * Return NULL if all the token were already consume. */
static inline char*
lex_next_token(struct lex* lex)
{
  char* token = NULL;
  assert(lex);
  token = strtok_r(lex->string, lex->delimiters, &lex->saveptr);
  lex->string = NULL;
  return token;
}

/* Take the next token of the lexer and parse it as a float.
 * Return false if no token exists or if it is not a float. */
static inline bool
lex_parse_float(struct lex* lex, float* out_f, char** out_token)
{
  char* ptr = NULL;
  char* token = NULL;
  float f = 0.f;
  bool no_error = true;

  assert(out_f && out_token);

  token = lex_next_token(lex);
  if(!token)
    goto error;

  f = strtof(token, &ptr);
  if(*ptr != '\0')
    goto error;

exit:
  *out_f = f;
  *out_token = token;
  return no_error;

error:
  no_error = false;
  goto exit;
}

/* Take the next token of the lexer and parse it as a long int.
 * Return false if no token exists or if it is not a long int. */
static inline bool
lex_parse_long_int(struct lex* lex, long int* out_i, char** out_token)
{
  char* token = NULL;
  char* ptr = NULL;
  long int i = 0;
  bool no_error = true;

  assert(out_i && out_token);

  token = lex_next_token(lex);
  if(!token)
    goto error;

  i = strtol(token, &ptr, 10);
  if(*ptr != '\0')
    goto error;

exit:
  *out_i = i;
  *out_token = token;
  return no_error;

error:
  no_error = false;
  goto exit;
}

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
/* Check that the name parameter is a valid group name. Ony alpha numeric, ':'
 * and '_' chars are allowed. */
static bool
is_name(const char* name)
{
  const char* ptr = name;
  bool no_error = true;

  assert(name);

  if(*ptr=='\0')
    goto error;

  while(*ptr!='\0' && no_error) {
    const char c = *ptr;
    no_error = isalpha(c) || isdigit(c) || c=='_' || c==':';
    ++ptr;
  }

exit:
  return no_error;

error:
  no_error = false;
  goto exit;
}

/* Set the end id of the element ranges of the last group registered in obj.
 * Return true if a group was flushed and false otherwise. */
static bool
flush_group(struct rsrc_wavefront_obj* wobj)
{
  group_t* group = NULL;
  size_t len = 0;

  assert(wobj);

  SL(vector_length(wobj->group_list, &len));
  if(len == 0)
    return false;

  SL(vector_at(wobj->group_list, len-1, (void**)&group));
  SL(vector_length(wobj->point_list, &len));
  group->point_range.end = len;
  SL(vector_length(wobj->line_list, &len));
  group->line_range.end = len;
  SL(vector_length(wobj->face_list, &len));
  group->face_range.end = len;
  return true;
}

/* Set the end id of the element ranges of the last group registered in obj.
 * Return true if a group was flushed and false otherwise. */
static bool
flush_smooth_group(struct rsrc_wavefront_obj* wobj)
{
  smooth_group_t* sgroup = NULL;
  size_t len = 0;

  assert(wobj);

  SL(vector_length(wobj->smooth_group_list, &len));
  if(len == 0)
    return false;

  SL(vector_at(wobj->smooth_group_list, len-1, (void**)&sgroup));
  SL(vector_length(wobj->face_list, &len));
  sgroup->face_range.end = len;
  return true;
}

/* Set the end id of the element ranges of the last group registered in obj.
 * Return true if a group was flushed and false otherwise. */
static bool
flush_usemtl(struct rsrc_wavefront_obj* wobj)
{
  mtl_t* mtl = NULL;
  size_t len = 0;

  assert(wobj);

  SL(vector_length(wobj->mtl_list, &len));
  if(len == 0)
    return false;

  SL(vector_at(wobj->mtl_list, len-1, (void**)&mtl));
  SL(vector_length(wobj->point_list, &len));
  mtl->point_range.end = len;
  SL(vector_length(wobj->line_list, &len));
  mtl->line_range.end = len;
  SL(vector_length(wobj->face_list, &len));
  mtl->face_range.end = len;
  return true;
}

/* Parse 3 floats. Check the eol. */
static enum rsrc_error
parse_xyz
  (struct rsrc_context* ctxt UNUSED,
   struct lex* lex,
   struct sl_vector* vec)
{
  float f3[3];
  char* token = NULL;
  enum sl_error sl_err = SL_NO_ERROR;
  enum rsrc_error err = RSRC_NO_ERROR;
  bool is_pushed = false;
  bool no_error = false;

  assert(vec);

  no_error =
    lex_parse_float(lex, f3 + 0, &token) &&
    lex_parse_float(lex, f3 + 1, &token) &&
    lex_parse_float(lex, f3 + 2, &token) &&
    !lex_next_token(lex);

  if(no_error == false) {
    err = RSRC_PARSING_ERROR;
    goto error;
  }

  sl_err = sl_vector_push_back(vec, f3);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }
  is_pushed = true;

exit:
  return err;

error:
  if(is_pushed)
    SL(vector_pop_back(vec));
  goto exit;
}

/* Parse 2 or 3 floats. Check the eol. */
static enum rsrc_error
parse_uvw
  (struct rsrc_context* ctxt UNUSED,
   struct lex* lex,
   struct sl_vector* vec)
{
  float f3[3];
  char* token = NULL;
  enum sl_error sl_err = SL_NO_ERROR;
  enum rsrc_error err = RSRC_NO_ERROR;
  bool is_pushed = false;
  bool no_error = false;

  assert(vec);

  no_error =
    lex_parse_float(lex, f3 + 0, &token) &&
    lex_parse_float(lex, f3 + 1, &token) &&
    (lex_parse_float(lex, f3 + 2, &token) || !token) &&
    !lex_next_token(lex);

  if(no_error == false) {
    err = RSRC_PARSING_ERROR;
    goto error;
  }

  /* !token => no w component. Set it to 0. */
  if(!token)
    f3[2] = 0.f;

  sl_err = sl_vector_push_back(vec, f3);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }
  is_pushed = true;

exit:
  return err;

error:
  if(is_pushed)
    SL(vector_pop_back(vec));
  goto exit;
}

/* Parse the coords ids of a point element. */
static enum rsrc_error
parse_point_elmt
  (struct rsrc_context* ctxt,
   struct lex* lex,
   struct sl_vector* point_list)
{
  char* token = NULL;
  struct sl_vector* vertices = NULL;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  size_t nb_vertices = 0;
  long int v = 0;
  bool is_pushed = false;

  assert(lex && point_list);

  sl_err = sl_create_vector
    (sizeof(size_t), ALIGNOF(size_t), ctxt->allocator, &vertices);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }

  /* Parse the point vertices. */
  while(lex_parse_long_int(lex, &v, &token)) {
    sl_err = sl_vector_push_back(vertices, &v);
    if(sl_err != SL_NO_ERROR) {
      err = sl_to_rsrc_error(sl_err);
      goto error;
    }
    ++nb_vertices;
  }

  /* A point element must have at least 1 vertex. */
  if(nb_vertices == 0 || token) {
    err = RSRC_PARSING_ERROR;
    goto error;
  }

  /* Add the point to the list of points. */
  sl_err = sl_vector_push_back(point_list, &vertices);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }
  is_pushed = true;

exit:
  return err;

error:
  if(is_pushed)
    SL(vector_pop_back(point_list));
  if(vertices)
    SL(free_vector(vertices));
  goto exit;
}

static enum rsrc_error
parse_line_elmt
  (struct rsrc_context* ctxt,
   struct lex* lex,
   struct sl_vector* line_list)
{
  char* token = NULL;
  struct sl_vector* vertices = NULL;
  enum sl_error sl_err = SL_NO_ERROR;
  enum rsrc_error err = RSRC_NO_ERROR;
  size_t nb_vertices = 0;
  long int v = 0, vt = 0;
  bool is_pushed = false;

  assert(lex && line_list);

  sl_err = sl_create_vector
    (sizeof(line_t), ALIGNOF(line_t), ctxt->allocator, &vertices);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }

  sl_err = sl_vector_reserve(vertices, 2);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;

  }
  /* Parse the line vertices. */
  while((token = lex_next_token(lex)) != NULL) {
    line_t line;
    struct lex tmp_lex = { .delimiters = "/", .string = token };
    char* tmp_tkn;

    /* Parse int >> !( '/' >> int >> ) eol */
    if(!lex_parse_long_int(&tmp_lex, &v, &tmp_tkn)
    || (!lex_parse_long_int(&tmp_lex, &vt, &tmp_tkn) && tmp_tkn)
    || lex_next_token(&tmp_lex)) {
      err = RSRC_PARSING_ERROR;
      goto error;
    }
    assert(v >= 0 && vt >= 0);

    /* Add the vertex to the line element. */
    line.v = (size_t)v;
    line.vt = (size_t)vt;
    sl_err = sl_vector_push_back(vertices, &line);
    if(sl_err != SL_NO_ERROR) {
      err = sl_to_rsrc_error(sl_err);
      goto error;
    }
    ++nb_vertices;
  }

  /* A line must have at least 2 vertices. */
  if(nb_vertices < 2) {
    err = RSRC_PARSING_ERROR;
    goto error;
  }

  /* Add the line to the list of lines. */
  sl_err = sl_vector_push_back(line_list, vertices);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);;
    goto error;
  }
  is_pushed = true;

exit:
  return err;

error:
  if(is_pushed)
    SL(vector_pop_back(line_list));
  if(vertices)
    SL(free_vector(vertices));
  goto exit;
}

static enum rsrc_error
parse_face_elmt
  (struct rsrc_context* ctxt,
   struct lex* lex,
   struct sl_vector* face_list)
{
  char* token = NULL;
  struct sl_vector* vertices = NULL;
  enum sl_error sl_err = SL_NO_ERROR;
  enum rsrc_error err = RSRC_NO_ERROR;
  size_t nb_vertices = 0;
  long int v = 0, vt = 0, vn = 0;
  bool is_pushed = false;

  assert(lex && face_list);

  sl_err = sl_create_vector
    (sizeof(face_t), ALIGNOF(face_t), ctxt->allocator, &vertices);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }

  /* We assume that the face is a triangle. */
  sl_err = sl_vector_reserve(vertices, 3);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }

  /*  Parse the face vertices. */
  while((token = lex_next_token(lex)) != NULL) {
    face_t face;
    struct lex tmp_lex = { .delimiters = "/", .string = token };
    const bool no_tex = strstr(token, "//") != NULL;
    char* tmp_tkn;

    /* Parse int >> !( '/' >> !int >> !'/' >> int >> ) >> eol */
    if(!lex_parse_long_int(&tmp_lex, &v, &tmp_tkn)) {
      err = RSRC_PARSING_ERROR;
      goto error;
    }
    if(no_tex) {
      if(!lex_parse_long_int(&tmp_lex, &vn, &tmp_tkn)
      || lex_next_token(&tmp_lex)) {
        err = RSRC_PARSING_ERROR;
        goto error;
      }
    } else {
      if(!lex_parse_long_int(&tmp_lex, &vt, &tmp_tkn)
      || (!lex_parse_long_int(&tmp_lex, &vn, &tmp_tkn) && tmp_tkn)) {
        err = RSRC_PARSING_ERROR;
        goto error;
      }
    }
    assert(v >= 0 && vt >= 0 && vn >= 0);

    /* Add the vertex to the face element. */
    face.v = (size_t)v;
    face.vt = (size_t)vt;
    face.vn = (size_t)vn;
    sl_err = sl_vector_push_back(vertices, &face);
    if(sl_err != SL_NO_ERROR) {
      err = sl_to_rsrc_error(sl_err);
      goto error;
    }
    ++nb_vertices;
  }

  /* A face must have at least 3 vertices. */
  if(nb_vertices < 3) {
    err = RSRC_PARSING_ERROR;
    goto error;
  }

  /* Add the face to the list of faces. */
  sl_err = sl_vector_push_back(face_list, &vertices);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }
  is_pushed = true;

exit:
  return err;

error:
  if(is_pushed)
    SL(vector_pop_back(face_list));
  if(vertices)
    SL(free_vector(vertices));
  goto exit;
}

static enum rsrc_error
parse_group(struct lex* lex, struct rsrc_wavefront_obj* wobj)
{
  group_t group;
  struct sl_vector* name_list = NULL;
  char* token = NULL;
  char* name = NULL;
  size_t i = 0;
  size_t len = 0;
  size_t nb_names = 0;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool is_pushed = false;

  assert(lex && wobj);

  /* flush the previous group. */
  flush_group(wobj);

  sl_err = sl_create_vector
    (sizeof(char*), ALIGNOF(char*), wobj->ctxt->allocator, &name_list);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }

  /* We assume that at most 2 group names will be set. */
  sl_err = sl_vector_reserve(name_list, 2);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }

  /* Parse the group names. */
  while((token = lex_next_token(lex)) != NULL) {
    size_t token_len = 0;
    if(!is_name(token)) {
      err = RSRC_PARSING_ERROR;
      goto error;
    }

    token_len = strlen(token) + 1; /* +1 <=> null terminated character. */
    name = MEM_ALLOC(wobj->ctxt->allocator, token_len*sizeof(char));
    if(!name) {
      err = RSRC_MEMORY_ERROR;
      goto error;
    }
    name = strncpy(name, token, token_len);

    sl_err = sl_vector_push_back(name_list, &name);
    if(sl_err != SL_NO_ERROR) {
      err = sl_to_rsrc_error(sl_err);
      goto error;
    }
    name = NULL;
    ++nb_names;
  }

  /* The group must have at least 1 valid name. */
  if(nb_names < 1) {
    err = RSRC_PARSING_ERROR;
    goto error;
  }

  group.name_list = name_list;
  SL(vector_length(wobj->point_list, &len));
  group.point_range.begin = len;
  SL(vector_length(wobj->line_list, &len));
  group.line_range.begin = len;
  SL(vector_length(wobj->face_list, &len));
  group.face_range.begin = len;
  sl_err = sl_vector_push_back(wobj->group_list, &group);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }
  is_pushed = true;

exit:
  return err;

error:
  if(is_pushed)
    SL(vector_pop_back(wobj->group_list));
  if(name_list) {
    void* buffer = NULL;
    SL(vector_buffer(name_list, &len, NULL, NULL, &buffer));
    for(i = 0; i < len; ++i)
      MEM_FREE(wobj->ctxt->allocator, ((char**)buffer)[i]);
    SL(free_vector(name_list));
  }
  if(name)
    MEM_FREE(wobj->ctxt->allocator, name);
  goto exit;
}

static enum rsrc_error
parse_smooth_group(struct lex* lex, struct rsrc_wavefront_obj* wobj)
{
  smooth_group_t sgroup;
  char* token = NULL;
  char* ptr = NULL;
  size_t len = 0;
  long int i = 0;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool no_error = true;
  bool is_on = false;
  bool is_pushed = false;

  assert(lex && wobj);

  /* Flush the previous smooth group. */
  flush_smooth_group(wobj);

  if((token = lex_next_token(lex)) == NULL) {
    err = RSRC_PARSING_ERROR;
    goto error;
  }

  /* Convert the token into long int */
  i = strtol(token, &ptr, 10);
  if(ptr != token) {
    no_error = (*ptr == '\0');
    is_on = i > 0;
  } else {
    is_on = strcmp(token, "on") == 0;
    no_error = is_on || strcmp(token, "off") == 0;
  }

  if(!no_error || lex_next_token(lex)) {
    err =  RSRC_PARSING_ERROR;
    goto error;
  }

  /* Create the smooth group and register it. */
  SL(vector_length(wobj->face_list, &len));
  sgroup.is_on = is_on;
  sgroup.face_range.begin = len;
  sgroup.face_range.end = 0;
  sl_err = sl_vector_push_back(wobj->smooth_group_list, &sgroup);
  if(sl_err != SL_NO_ERROR) {
    err = sl_to_rsrc_error(sl_err);
    goto error;
  }
  is_pushed = true;

exit:
  return err;

error:
  if(is_pushed)
    SL(vector_pop_back(wobj->smooth_group_list));
  goto exit;
}

static enum rsrc_error
parse_mtllib
  (struct rsrc_context* ctxt,
   struct lex* lex,
   struct sl_vector* mtllib_list)
{
  char* token = NULL;
  size_t nb_libs = 0;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  assert(lex && mtllib_list);

  while((token = lex_next_token(lex)) != NULL) {
    const size_t token_len = strlen(token) + 1; /* +1 <=> null character. */
    char* libname = MEM_ALLOC(ctxt->allocator, token_len * sizeof(char));
    if(!libname) {
      err = RSRC_MEMORY_ERROR;
      goto error;
    }
    libname = strncpy(libname, token, token_len);

    sl_err = sl_vector_push_back(mtllib_list, &libname);
    if(sl_err != SL_NO_ERROR) {
      err = sl_to_rsrc_error(sl_err);
      goto error;
    }
    libname = NULL;
    ++nb_libs;
  }

  if(nb_libs == 0) {
    err = RSRC_PARSING_ERROR;
    goto error;
  }

exit:
  return err;

error:
  while(nb_libs) {
    size_t len = 0;
    char* libname = NULL;
    SL(vector_length(mtllib_list, &len));
    SL(vector_at(mtllib_list, len-1, (void**)&libname));
    MEM_FREE(ctxt->allocator, libname);
    SL(vector_pop_back(mtllib_list));
    --nb_libs;
  }
  goto exit;
}

static enum rsrc_error
parse_mtl(struct lex* lex, struct rsrc_wavefront_obj* wobj)
{
  mtl_t mtl;
  char* token = NULL;
  size_t len = 0;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;
  bool is_pushed = false;

  assert(lex && wobj);

  /* Flush the previous usemtl. */
  flush_usemtl(wobj);

  token = lex_next_token(lex);
  if(!token || lex_next_token(lex)) {
    err = RSRC_PARSING_ERROR;
    goto error;
  }

  len = strlen(token) + 1;
  mtl.name = MEM_ALLOC(wobj->ctxt->allocator, len*sizeof(char));
  if(!mtl.name) {
    err = RSRC_MEMORY_ERROR;
    goto error;
  }
  mtl.name = strncpy(mtl.name, token, len);

  SL(vector_length(wobj->point_list, &len));
  mtl.point_range.begin = len;
  SL(vector_length(wobj->line_list, &len));
  mtl.line_range.begin = len;
  SL(vector_length(wobj->face_list, &len));
  mtl.face_range.begin = len;

  sl_err = sl_vector_push_back(wobj->mtl_list, &mtl);
  if(sl_err != SL_NO_ERROR) {
    err = RSRC_MEMORY_ERROR;
    goto error;
  }
  is_pushed = true;

exit:
  return err;

error:
  if(is_pushed)
    SL(vector_pop_back(wobj->mtl_list));
  if(mtl.name)
    MEM_FREE(wobj->ctxt->allocator, mtl.name);
  goto exit;
}

static enum rsrc_error
clear_wavefront_obj(struct rsrc_wavefront_obj* wobj)
{
  void* buf = NULL;
  size_t len = 0;
  size_t i = 0;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  assert(wobj);

  #define CLEAR_VECTOR(v) \
    do { \
      sl_err = sl_clear_vector(v); \
      if(sl_err != SL_NO_ERROR) { \
        err = sl_to_rsrc_error(sl_err); \
        goto error; \
      } \
    } while(0)

  #define VECTOR_BUFFER(vec, len, buf) \
    do { \
      sl_err = sl_vector_buffer(vec, &len, NULL, NULL, (void**)&buf); \
      if(sl_err != SL_NO_ERROR) { \
        err = sl_to_rsrc_error(sl_err); \
        goto error; \
      } \
    } while(0)

  #define FREE_VECTOR(v) \
    do { \
      sl_err = sl_free_vector(v); \
      if(sl_err != SL_NO_ERROR) { \
        err = sl_to_rsrc_error(sl_err); \
        goto error; \
      } \
    } while(0)

  if(wobj->position_list)
    CLEAR_VECTOR(wobj->position_list);

  if(wobj->normal_list)
    CLEAR_VECTOR(wobj->normal_list);

  if(wobj->texcoord_list)
    CLEAR_VECTOR(wobj->texcoord_list);

  if(wobj->point_list) {
    VECTOR_BUFFER(wobj->line_list, len, buf);
    for(i = 0; i < len; ++i)
      FREE_VECTOR(((struct sl_vector**)buf)[i]);
    CLEAR_VECTOR(wobj->line_list);
  }

  if(wobj->line_list) {
    VECTOR_BUFFER(wobj->line_list, len, buf);
    for(i = 0; i < len; ++i)
      FREE_VECTOR(((struct sl_vector**)buf)[i]);
    CLEAR_VECTOR(wobj->line_list);
  }

  if(wobj->face_list) {
    VECTOR_BUFFER(wobj->face_list, len, buf);
    for(i = 0; i < len; ++i)
      FREE_VECTOR(((struct sl_vector**)buf)[i]);
    CLEAR_VECTOR(wobj->face_list);
  }

  if(wobj->group_list) {
    VECTOR_BUFFER(wobj->group_list, len, buf);
    for(i = 0; i < len; ++i) {
      group_t* grp = (group_t*)buf + i;
      void* name_list = NULL;
      size_t nb_names = 0;
      size_t name_id = 0;

      VECTOR_BUFFER(grp->name_list, nb_names, name_list);
      for(name_id = 0; name_id < nb_names; ++name_id)
        MEM_FREE(wobj->ctxt->allocator, ((char**)name_list)[name_id]);

      FREE_VECTOR(grp->name_list);
    }
    CLEAR_VECTOR(wobj->group_list);
  }

  if(wobj->smooth_group_list)
    CLEAR_VECTOR(wobj->smooth_group_list);

  if(wobj->mtllib_list) {
    VECTOR_BUFFER(wobj->mtllib_list, len, buf);
    for(i = 0; i < len; ++i)
      MEM_FREE(wobj->ctxt->allocator, ((char**)buf)[i]);
    CLEAR_VECTOR(wobj->mtllib_list);
  }

  if(wobj->mtl_list) {
    VECTOR_BUFFER(wobj->mtl_list, len, buf);
    for(i = 0; i < len; ++i)
      MEM_FREE(wobj->ctxt->allocator, ((mtl_t*)buf)[i].name);
    CLEAR_VECTOR(wobj->mtl_list);
  }

  #undef CLEAR_VECTOR
  #undef VECTOR_BUFFER
  #undef FREE_VECTOR

exit:
  return err;

error:
  goto exit;
}

static enum rsrc_error
parse_wavefront_obj
  (struct rsrc_wavefront_obj* wobj,
   const char* path,
   char* filecontent)
{
  enum rsrc_error err = RSRC_NO_ERROR;
  struct lex lex_eol = { .delimiters = "\n", .string = filecontent };
  char* line = NULL;
  char* end_line = NULL;
  size_t line_id = 0;

  assert(wobj && path && filecontent);

  line = lex_next_token(&lex_eol);
  end_line = line;  /* Pointer toward the eol char of the previous persed line. */
  line_id = 1;
  while(line) {
    /* Find the tokens in the line with respect to the space delimiter */
    struct lex lex_space = { .delimiters = " ", .string = line };
    char* token = NULL;

    /* Update the line_id with respect to the difference of bytes between the
     * current line and the previous one. N bytes => N eol. */
    line_id += line - end_line;
    end_line = line + strlen(line);

    token = lex_next_token(&lex_space);
    while(token) {
      if(*token == '#') { /* Comment. */
        break;
      } else if(!strcmp(token, "v")) { /* Vertex position. */
        err = parse_xyz(wobj->ctxt, &lex_space, wobj->position_list);
        if(err != RSRC_NO_ERROR)
          goto error;
      } else if(!strcmp(token, "vn")) { /* Vertex normal. */
        err = parse_xyz(wobj->ctxt, &lex_space, wobj->normal_list);
        if(err != RSRC_NO_ERROR)
          goto error;
      } else if(!strcmp(token, "vt")) { /* Vertex texture coordinates. */
        err = parse_uvw(wobj->ctxt, &lex_space, wobj->texcoord_list);
        if(err != RSRC_NO_ERROR)
          goto error;
      } else if(!strcmp(token, "p")) { /* Point element. */
        err = parse_point_elmt(wobj->ctxt, &lex_space, wobj->point_list);
        if(err != RSRC_NO_ERROR)
          goto error;
      } else if(!strcmp(token, "l")) { /* Line element. */
        err = parse_line_elmt(wobj->ctxt, &lex_space, wobj->line_list);
        if(err != RSRC_NO_ERROR)
          goto error;
      } else if(!strcmp(token, "f")
             || !strcmp(token, "fo")) { /* Face element. */
        err = parse_face_elmt(wobj->ctxt, &lex_space, wobj->face_list);
        if(err != RSRC_NO_ERROR)
          goto error;
      } else if(!strcmp(token, "g")) { /* Grouping. */
        err = parse_group(&lex_space, wobj);
        if(err != RSRC_NO_ERROR)
          goto error;
      } else if(!strcmp(token, "s")) { /* Smooth group. */
        err = parse_smooth_group(&lex_space, wobj);
        if(err != RSRC_NO_ERROR)
          goto error;
      } else if(!strcmp(token, "mtllib")) { /* Mtl libraray render attrib. */
        err = parse_mtllib(wobj->ctxt, &lex_space, wobj->mtllib_list);
        if(err != RSRC_NO_ERROR)
          goto error;
      } else if(!strcmp(token, "usemtl")) { /* Use mtl render attrib. */
        err = parse_mtl(&lex_space, wobj);
        if(err != RSRC_NO_ERROR)
          goto error;
      } else {
        err = RSRC_PARSING_ERROR;
        goto error;
      }
      token = lex_next_token(&lex_space);
    }
    line = lex_next_token(&lex_eol);
  }
  flush_group(wobj);
  flush_smooth_group(wobj);
  flush_usemtl(wobj);

exit:
  return err;

error:
  fprintf(stderr, "%s:%zd: error: parsing failed.\n", path, line_id);
  err = clear_wavefront_obj(wobj);
  assert(err == RSRC_NO_ERROR);
  goto exit;
}

/*******************************************************************************
 *
 * Implementation of the public functions of the wavefront obj data structure.
 *
 ******************************************************************************/
EXPORT_SYM enum rsrc_error
rsrc_create_wavefront_obj
  (struct rsrc_context* ctxt,
   struct rsrc_wavefront_obj** out_wobj)
{
  struct rsrc_wavefront_obj* wobj = NULL;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!ctxt || ! out_wobj) {
    err = RSRC_INVALID_ARGUMENT;
    goto error;
  }

  wobj = MEM_CALLOC(ctxt->allocator, 1, sizeof(struct rsrc_wavefront_obj));
  if(!wobj) {
    err = RSRC_MEMORY_ERROR;
    goto error;
  }
  wobj->ctxt = ctxt;
  RSRC(context_ref_get(ctxt));

  #define CREATE_VECTOR(v, t) \
    do { \
      sl_err = sl_create_vector \
        (sizeof(t), ALIGNOF(t), wobj->ctxt->allocator, &v); \
      if(sl_err != SL_NO_ERROR) { \
        err = sl_to_rsrc_error(sl_err); \
        goto error; \
      } \
    } while(0)

  CREATE_VECTOR(wobj->position_list, float[3]);
  CREATE_VECTOR(wobj->normal_list, float[3]);
  CREATE_VECTOR(wobj->texcoord_list, float[3]);
  CREATE_VECTOR(wobj->point_list, struct sl_vector*);
  CREATE_VECTOR(wobj->line_list, struct sl_vector*);
  CREATE_VECTOR(wobj->face_list, struct sl_vector*);
  CREATE_VECTOR(wobj->group_list, group_t);
  CREATE_VECTOR(wobj->smooth_group_list, smooth_group_t);
  CREATE_VECTOR(wobj->mtllib_list, char*);
  CREATE_VECTOR(wobj->mtl_list, mtl_t);

  #undef CREATE_VECTOR

exit:
  if(out_wobj)
    *out_wobj = wobj;
  return err;

error:
  if(wobj) {
    err = rsrc_free_wavefront_obj(wobj);
    assert(err == RSRC_NO_ERROR);
    MEM_FREE(wobj->ctxt->allocator, wobj);
    wobj = NULL;
  }
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_free_wavefront_obj(struct rsrc_wavefront_obj* wobj)
{
  struct rsrc_context* ctxt = NULL;
  enum rsrc_error err = RSRC_NO_ERROR;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!wobj) {
    err = RSRC_INVALID_ARGUMENT;
    goto error;
  }

  err = clear_wavefront_obj(wobj);
  if(err != RSRC_NO_ERROR)
    goto error;

  #define FREE_VECTOR(v) \
    do { \
      if(v) { \
        sl_err = sl_free_vector(v); \
        if(sl_err != SL_NO_ERROR) { \
          err = sl_to_rsrc_error(sl_err); \
          goto error; \
        } \
      } \
    } while(0)

  FREE_VECTOR(wobj->position_list);
  FREE_VECTOR(wobj->normal_list);
  FREE_VECTOR(wobj->texcoord_list);
  FREE_VECTOR(wobj->point_list);
  FREE_VECTOR(wobj->line_list);
  FREE_VECTOR(wobj->face_list);
  FREE_VECTOR(wobj->group_list);
  FREE_VECTOR(wobj->smooth_group_list);
  FREE_VECTOR(wobj->mtllib_list);
  FREE_VECTOR(wobj->mtl_list);

  #undef FREE_VECTOR

  ctxt = wobj->ctxt;
  MEM_FREE(ctxt->allocator, wobj);
  RSRC(context_ref_put(ctxt));

exit:
  return err;

error:
  goto exit;
}

EXPORT_SYM enum rsrc_error
rsrc_load_wavefront_obj
  (struct rsrc_wavefront_obj* wobj,
   const char* path)
{
  FILE* fptr = NULL;
  char* file_content = NULL;
  long file_len = 0;
  size_t nb_bytes = 0;
  enum rsrc_error err = RSRC_NO_ERROR;

  if(!wobj || !path) {
    err = RSRC_NO_ERROR;
    goto error;
  }

  fptr = fopen(path, "r");
  if(!fptr) {
    err = RSRC_IO_ERROR;
    goto error;
  }

  if(fseek(fptr, 0, SEEK_END) != 0) {
    err = RSRC_IO_ERROR;
    goto error;
  }

  if((file_len = ftell(fptr)) < 0) {
    err = RSRC_IO_ERROR;
    goto error;
  }

  if(fseek(fptr, 0L, SEEK_SET) != 0) {
    err = RSRC_IO_ERROR;
    goto error;
  }

  /* +1 <=> null terminated character. */
  file_content = MEM_ALLOC(wobj->ctxt->allocator, file_len * sizeof(char) + 1);
  if(!file_content) {
    err = RSRC_MEMORY_ERROR;
    goto error;
  }

  nb_bytes = fread(file_content, sizeof(char), file_len, fptr);
  if(nb_bytes != (size_t)file_len) {
    err = RSRC_IO_ERROR;
    goto error;
  }

  file_content[file_len] = '\0';
  fclose(fptr);
  fptr = NULL;

  err = clear_wavefront_obj(wobj);
  if(err != RSRC_NO_ERROR)
    goto error;

  err = parse_wavefront_obj(wobj, path, file_content);
  if(err != RSRC_NO_ERROR)
    goto error;

exit:
  if(file_content)
    MEM_FREE(wobj->ctxt->allocator, file_content);
  return err;

error:
  goto exit;
}

