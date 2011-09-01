#include "renderer/rdr_material.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_model.h"
#include "renderer/rdr_model_instance.h"
#include "renderer/rdr_system.h"
#include "utest/utest.h"
#include "window_manager/wm.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool
is_driver_null(const char* name)
{
  const char* null_driver_name = "librbnull.so";
  char* p = NULL;

  if(name == NULL)
    return false;

  p = strstr(name, null_driver_name);

  return (p != NULL)
    && (p == name ? true : *(p-1) == '/')
    && (*(p + strlen(null_driver_name)) == '\0');
}

static void
test_rdr_system(const char* driver_name)
{
  struct rdr_system* sys = NULL;

  CHECK(rdr_create_system(NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_system(NULL, &sys), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_system("__INVALID_DRIVER__", NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_system("__INVALID_DRIVER__", &sys), RDR_DRIVER_ERROR);
  CHECK(rdr_create_system(driver_name, &sys), RDR_NO_ERROR);
  CHECK(rdr_free_system(NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_free_system(sys), RDR_NO_ERROR);
}

static void
test_rdr_mesh(const char* driver_name)
{
  struct rdr_system* sys = NULL;
  struct rdr_mesh* mesh = NULL;
  const float vertices1[] = { 0.f, 0.f, 0.f };
  const float vertices2[] = { 0.f, 0.f, 0.f, 1.f, 1.f, 1.f };
  const unsigned int indices1[] = { 0, 1, 2 };
  const unsigned int indices2[] = { 0, 1, 2, 3 };
  const struct rdr_mesh_attrib mesh_attr[] = {
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT2 },
    { .usage = RDR_ATTRIB_COLOR, .type = RDR_FLOAT },
    { .usage = RDR_ATTRIB_UNKNOWN, .type = RDR_FLOAT }
  };
  const struct rdr_mesh_attrib mesh_attr2[] = {
    { .usage = RDR_ATTRIB_NORMAL, .type = RDR_UNKNOWN_TYPE }
  };
  const struct rdr_mesh_attrib mesh_attr3[] = {
    { .usage = RDR_ATTRIB_UNKNOWN, .type = RDR_FLOAT }
  };

  CHECK(rdr_create_system(driver_name, &sys), RDR_NO_ERROR);

  CHECK(rdr_create_mesh(NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_mesh(NULL, &mesh), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_mesh(sys, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_mesh(sys, &mesh), RDR_NO_ERROR);

  CHECK(rdr_mesh_data(NULL, NULL, 0, NULL, 0, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(sys, NULL, 0, NULL, 0, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(NULL, mesh, 0, NULL, 0, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(NULL, NULL, 1, NULL, 0, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(NULL, NULL, 0, mesh_attr, 0, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(NULL, NULL, 0, NULL, 0, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(sys, mesh, 0, mesh_attr, sizeof(vertices1), vertices1),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(sys, mesh, 2, NULL, sizeof(vertices1), vertices1),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(sys, mesh, 2, mesh_attr, 0, vertices1),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(sys, mesh, 2, mesh_attr, sizeof(vertices1), NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(sys, mesh, 2, NULL, sizeof(vertices1), NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(sys, mesh, 1, mesh_attr, sizeof(vertices1), vertices1),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(NULL, mesh, 2, mesh_attr, sizeof(vertices1), vertices1),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(NULL, NULL, 2, mesh_attr, sizeof(vertices1), vertices1),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(sys, mesh, 1, mesh_attr2, sizeof(vertices1), vertices1),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(sys, mesh, 1, mesh_attr3, sizeof(vertices1), vertices1),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_data(sys, mesh, 0, mesh_attr, 0, vertices1),
        RDR_NO_ERROR);
  CHECK(rdr_mesh_data(sys, mesh, 0, NULL, 0, NULL),
        RDR_NO_ERROR);
  CHECK(rdr_mesh_data(sys, mesh, 2, mesh_attr, sizeof(vertices1), vertices1),
        RDR_NO_ERROR);
  CHECK(rdr_mesh_data(sys, mesh, 2, mesh_attr, sizeof(vertices2), vertices2),
        RDR_NO_ERROR);
  CHECK(rdr_mesh_data(sys, mesh, 1, mesh_attr, sizeof(vertices2), vertices2),
        RDR_NO_ERROR);

  CHECK(rdr_mesh_indices(NULL, NULL, 3, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_indices(sys, NULL, 3, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_indices(NULL, mesh, 3, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_indices(sys, mesh, 4, indices2), RDR_INVALID_ARGUMENT);
  CHECK(rdr_mesh_indices(sys, mesh, 3, indices1), RDR_NO_ERROR);
  CHECK(rdr_mesh_indices(sys, mesh, 0, indices1), RDR_NO_ERROR);

  CHECK(rdr_free_mesh(NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_free_mesh(NULL, mesh), RDR_INVALID_ARGUMENT);
  CHECK(rdr_free_mesh(sys, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_free_mesh(sys, mesh), RDR_NO_ERROR);
  CHECK(rdr_free_system(sys), RDR_NO_ERROR);
}

static void
test_rdr_material(const char* driver_name)
{
  struct rdr_system* sys = NULL;
  struct rdr_material* mtr = NULL;
  const char* bad_source = "__INVALID_SOURCE_";
  const char* good_source =
    "#version 330\n"
    "void main()\n"
    "{\n"
      "gl_Position = vec4(0.f,0.f,0.f,0.f);\n"
    "}\n";
  const char* log = NULL;
  const char* sources[RDR_NB_SHADER_USAGES];
  bool null_driver = is_driver_null(driver_name);

  memset(sources, 0, sizeof(const char*) * RDR_NB_SHADER_USAGES);

  CHECK(rdr_create_system(driver_name, &sys), RDR_NO_ERROR);

  CHECK(rdr_create_material(NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_material(sys, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_material(NULL, &mtr), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_material(sys, &mtr), RDR_NO_ERROR);

  sources[RDR_VERTEX_SHADER] = bad_source;
  CHECK(rdr_material_program(NULL, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_material_program(sys, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_material_program(NULL, mtr, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_material_program(sys, mtr, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_material_program(NULL, NULL, sources), RDR_INVALID_ARGUMENT);
  CHECK(rdr_material_program(sys, NULL, sources), RDR_INVALID_ARGUMENT);
  CHECK(rdr_material_program(NULL, mtr, sources), RDR_INVALID_ARGUMENT);
  if(null_driver == false)
    CHECK(rdr_material_program(sys, mtr, sources), RDR_DRIVER_ERROR);

  CHECK(rdr_get_material_log(NULL, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_material_log(sys, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_material_log(NULL, mtr, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_material_log(sys, mtr, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_material_log(NULL, NULL, &log), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_material_log(sys, NULL, &log), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_material_log(NULL, mtr, &log), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_material_log(sys, mtr, &log), RDR_NO_ERROR);

  sources[RDR_VERTEX_SHADER] = good_source;
  CHECK(rdr_material_program(sys, mtr, sources), RDR_NO_ERROR);

  CHECK(rdr_free_material(NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_free_material(sys, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_free_material(NULL, mtr), RDR_INVALID_ARGUMENT);
  CHECK(rdr_free_material(sys, mtr), RDR_NO_ERROR);

  CHECK(rdr_free_system(sys), RDR_NO_ERROR);
}

static void
test_rdr_model(const char* driver_name)
{
  struct rdr_system* sys = NULL;
  struct rdr_material* mtr = NULL;
  struct rdr_mesh* mesh = NULL;
  struct rdr_model* model0 = NULL;
  struct rdr_model* model1 = NULL;
  const char* vs_source =
    "#version 330\n"
    "void main()\n"
    "{\n"
      "gl_Position = vec4(0.f,0.f,0.f,0.f);\n"
    "}\n";
  const char* sources[RDR_NB_SHADER_USAGES];
  const float data[] = { 0.f, 0.f, 0.f, 1.f, 1.f, 1.f };
  const struct rdr_mesh_attrib attr0[] = {
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT2 },
    { .usage = RDR_ATTRIB_COLOR, .type = RDR_FLOAT }
  };
  const struct rdr_mesh_attrib attr1[] = {
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT2 }
  };

  memset(sources, 0, sizeof(const char*) * RDR_NB_SHADER_USAGES);
  sources[RDR_VERTEX_SHADER] = vs_source;

  CHECK(rdr_create_system(driver_name, &sys), RDR_NO_ERROR);
  CHECK(rdr_create_mesh(sys, &mesh), RDR_NO_ERROR);
  CHECK(rdr_mesh_data(sys, mesh, 2, attr0, sizeof(data), data), RDR_NO_ERROR);
  CHECK(rdr_create_material(sys, &mtr), RDR_NO_ERROR);
  CHECK(rdr_material_program(sys, mtr, sources), RDR_NO_ERROR);

  CHECK(rdr_create_model(NULL, NULL, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model(sys, NULL, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model(NULL, mesh, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model(sys, mesh, NULL, NULL), RDR_INVALID_ARGUMENT);

  CHECK(rdr_create_model(NULL, NULL, mtr, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model(sys, NULL, mtr, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model(NULL, mesh, mtr, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model(sys, mesh, mtr, NULL), RDR_INVALID_ARGUMENT);

  CHECK(rdr_create_model(NULL, NULL, mtr, &model0), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model(sys, NULL, mtr, &model0), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model(NULL, mesh, mtr, &model0), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model(sys, mesh, mtr, &model0), RDR_NO_ERROR);
  CHECK(rdr_create_model(sys, mesh, mtr, &model1), RDR_NO_ERROR);

  CHECK(rdr_model_mesh(NULL, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_mesh(sys, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_mesh(NULL, model0, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_mesh(sys, model0, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_mesh(NULL, NULL, mesh), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_mesh(sys, NULL, mesh), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_mesh(NULL, model0, mesh), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_mesh(sys, model0, mesh), RDR_NO_ERROR);

  CHECK(rdr_mesh_data(sys, mesh, 1, attr1, sizeof(data), data), RDR_NO_ERROR);

  CHECK(rdr_free_model(NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_free_model(sys, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_free_model(NULL, model0), RDR_INVALID_ARGUMENT);
  CHECK(rdr_free_model(sys, model0), RDR_NO_ERROR);

  CHECK(rdr_free_mesh(sys, mesh), RDR_NO_ERROR);
  CHECK(rdr_free_material(sys, mtr), RDR_NO_ERROR);
  CHECK(rdr_free_system(sys), RDR_NO_ERROR);
}

struct instance_cbk_data {
  struct rdr_model_instance_data* uniform_list;
  struct rdr_model_instance_data* attrib_list;
  size_t nb_uniforms;
  size_t nb_attribs;
};

static enum rdr_error
instance_cbk_func
  (struct rdr_system* sys,
   struct rdr_model_instance* instance,
   void* data)
{
  struct instance_cbk_data* cbk_data = data;

  assert(cbk_data != NULL);

  if(cbk_data->uniform_list) {
    free(cbk_data->uniform_list);
    cbk_data->uniform_list = NULL;
    cbk_data->nb_uniforms = 0;
  }

  if(cbk_data->attrib_list) {
    free(cbk_data->attrib_list);
    cbk_data->attrib_list = NULL;
    cbk_data->nb_attribs = 0;
  }

  CHECK(rdr_get_model_instance_uniforms(NULL, NULL, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_uniforms(sys, NULL, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_uniforms(NULL, instance, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_uniforms(sys, instance, NULL, NULL),
        RDR_INVALID_ARGUMENT);

  CHECK(rdr_get_model_instance_uniforms
        (NULL, NULL, &cbk_data->nb_uniforms, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_uniforms
        (sys, NULL, &cbk_data->nb_uniforms, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_uniforms
        (NULL, instance, &cbk_data->nb_uniforms, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_uniforms
        (sys, instance, NULL, NULL),
        RDR_INVALID_ARGUMENT);

  CHECK(rdr_get_model_instance_uniforms
        (NULL, NULL, NULL, cbk_data->uniform_list),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_uniforms
        (sys, NULL, NULL, cbk_data->uniform_list),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_uniforms
        (NULL, instance, NULL, cbk_data->uniform_list),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_uniforms
        (sys, instance, NULL, cbk_data->uniform_list),
        RDR_INVALID_ARGUMENT);

  CHECK(rdr_get_model_instance_uniforms
        (NULL, NULL, &cbk_data->nb_uniforms, cbk_data->uniform_list),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_uniforms
        (sys, NULL, &cbk_data->nb_uniforms, cbk_data->uniform_list),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_uniforms
        (NULL, instance, &cbk_data->nb_uniforms, cbk_data->uniform_list),
        RDR_INVALID_ARGUMENT);

  CHECK(rdr_get_model_instance_uniforms
        (sys, instance, &cbk_data->nb_uniforms, NULL),
        RDR_NO_ERROR);

  if(cbk_data->nb_uniforms > 0) {
    cbk_data->uniform_list = calloc
      (cbk_data->nb_uniforms, sizeof(struct instance_cbk_data));
    assert(NULL != cbk_data->uniform_list);

    CHECK(rdr_get_model_instance_uniforms
          (sys, instance, &cbk_data->nb_uniforms, cbk_data->uniform_list),
          RDR_NO_ERROR);
  }

  CHECK(rdr_get_model_instance_attribs(NULL, NULL, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_attribs(sys, NULL, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_attribs(NULL, instance, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_attribs(sys, instance, NULL, NULL),
        RDR_INVALID_ARGUMENT);

  CHECK(rdr_get_model_instance_attribs
        (NULL, NULL, &cbk_data->nb_attribs, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_attribs
        (sys, NULL, &cbk_data->nb_attribs, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_attribs
        (NULL, instance, &cbk_data->nb_attribs, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_attribs
        (sys, instance, NULL, NULL),
        RDR_INVALID_ARGUMENT);

  CHECK(rdr_get_model_instance_attribs
        (NULL, NULL, NULL, cbk_data->attrib_list),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_attribs
        (sys, NULL, NULL, cbk_data->attrib_list),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_attribs
        (NULL, instance, NULL, cbk_data->attrib_list),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_attribs
        (sys, instance, NULL, cbk_data->attrib_list),
        RDR_INVALID_ARGUMENT);

  CHECK(rdr_get_model_instance_attribs
        (NULL, NULL, &cbk_data->nb_attribs, cbk_data->attrib_list),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_attribs
        (sys, NULL, &cbk_data->nb_attribs, cbk_data->attrib_list),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_attribs
        (NULL, instance, &cbk_data->nb_attribs, cbk_data->attrib_list),
        RDR_INVALID_ARGUMENT);

  CHECK(rdr_get_model_instance_attribs
        (sys, instance, &cbk_data->nb_attribs, NULL),
        RDR_NO_ERROR);

  if(cbk_data->nb_attribs > 0) {
    cbk_data->attrib_list = calloc
      (cbk_data->nb_attribs, sizeof(struct instance_cbk_data));
    assert(NULL != cbk_data->attrib_list);

    CHECK(rdr_get_model_instance_attribs
          (sys, instance, &cbk_data->nb_attribs, cbk_data->attrib_list),
          RDR_NO_ERROR);
  }

  return RDR_NO_ERROR;
}

static void
test_rdr_model_instance(const char* driver_name)
{
  struct instance_cbk_data instance_cbk_data;
  struct rdr_system* sys = NULL;
  struct rdr_material* mtr = NULL;
  struct rdr_mesh* mesh = NULL;
  struct rdr_model* model = NULL;
  struct rdr_model_instance* inst = NULL;
  struct rdr_model_instance* inst1 = NULL;
  struct rdr_model_instance_callback_desc cbk_desc;
  struct rdr_model_instance_callback* cbk = NULL;
  const char* vs_source =
    "#version 330\n"
    "uniform mat4x4 rdr_viewproj;\n"
    "uniform float tmp;\n"
    "in vec4 rdr_position;\n"
    "in vec4 rdr_color; \n"
    "smooth out vec4 color; \n"
    "void main()\n"
    "{\n"
      "vec4 v = tmp * rdr_position;\n"
      "gl_Position = rdr_viewproj * v;\n"
      "color = rdr_color;\n"
    "}\n";
  const char* vs_source1 =
    "#version 330\n"
    "in vec2 rdr_position;\n"
    "in float rdr_color; \n"
    "void main()\n"
    "{\n"
      "gl_Position.x = rdr_position.x * rdr_color;\n"
      "gl_Position.yzw = vec3(0.f);\n"
    "}\n";
  const char* sources[RDR_NB_SHADER_USAGES];
  const float data[] = { 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f };
  const struct rdr_mesh_attrib attr0[] = {
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT4 },
    { .usage = RDR_ATTRIB_COLOR, .type = RDR_FLOAT4 }
  };
  const struct rdr_mesh_attrib attr1[] = {
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT4 }
  };
  const struct rdr_mesh_attrib attr2[] = {
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT2 },
    { .usage = RDR_ATTRIB_COLOR, .type = RDR_FLOAT },
    { .usage = RDR_ATTRIB_TEXCOORD, .type = RDR_FLOAT }
  };
  const float m44[16] = {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
  };
  float m[16];
  enum rdr_material_density mtr_density;
  struct rdr_rasterizer_desc rast;
  bool null_driver = is_driver_null(driver_name);

  memset(&instance_cbk_data, 0, sizeof(struct instance_cbk_data));
  memset(sources, 0, sizeof(const char*) * RDR_NB_SHADER_USAGES);
  sources[RDR_VERTEX_SHADER] = vs_source;

  CHECK(rdr_create_system(driver_name, &sys), RDR_NO_ERROR);
  CHECK(rdr_create_mesh(sys, &mesh), RDR_NO_ERROR);
  CHECK(rdr_mesh_data(sys, mesh, 2, attr0, sizeof(data), data), RDR_NO_ERROR);
  CHECK(rdr_create_material(sys, &mtr), RDR_NO_ERROR);
  CHECK(rdr_material_program(sys, mtr, sources), RDR_NO_ERROR);
  CHECK(rdr_create_model(sys, mesh, mtr, &model), RDR_NO_ERROR);

  CHECK(rdr_create_model_instance(NULL, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model_instance(sys, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model_instance(NULL, model, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model_instance(sys, model, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model_instance(NULL, NULL, &inst), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model_instance(sys, NULL, &inst), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model_instance(NULL, model, &inst), RDR_INVALID_ARGUMENT);
  CHECK(rdr_create_model_instance(sys, model, &inst), RDR_NO_ERROR);

  CHECK(rdr_mesh_data(sys, mesh, 1, attr1, sizeof(data), data), RDR_NO_ERROR);

  cbk_desc.data = (void*)&instance_cbk_data;
  cbk_desc.func = instance_cbk_func;
  CHECK(rdr_attach_model_instance_callback(NULL, NULL, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(sys, NULL, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(NULL, inst, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(sys, inst, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(NULL, NULL, &cbk_desc, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(sys, NULL, &cbk_desc, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(NULL, inst, &cbk_desc, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(sys, inst, &cbk_desc, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(NULL, NULL, NULL, &cbk),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(sys, NULL, NULL, &cbk),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(NULL, inst, NULL, &cbk),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(sys, inst, NULL, &cbk),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(NULL, NULL, &cbk_desc, &cbk),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(sys, NULL, &cbk_desc, &cbk),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(NULL, inst, &cbk_desc, &cbk),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_attach_model_instance_callback(sys, inst, &cbk_desc, &cbk),
        RDR_NO_ERROR);

  CHECK(rdr_mesh_data(sys, mesh, 1, attr1, sizeof(data), data), RDR_NO_ERROR);
  if(!null_driver) {
    CHECK(instance_cbk_data.nb_uniforms, 1);
    CHECK(strcmp(instance_cbk_data.uniform_list[0].name, "tmp"), 0);
    CHECK(instance_cbk_data.uniform_list[0].type, RDR_FLOAT);
    CHECK(instance_cbk_data.nb_attribs, 1);
    CHECK(strcmp(instance_cbk_data.attrib_list[0].name, "rdr_color"), 0);
    CHECK(instance_cbk_data.attrib_list[0].type, RDR_FLOAT4);
  }

  CHECK(rdr_mesh_data(sys, mesh, 2, attr0, sizeof(data), data), RDR_NO_ERROR);
  if(!null_driver) {
    CHECK(instance_cbk_data.nb_uniforms, 1);
    CHECK(strcmp(instance_cbk_data.uniform_list[0].name, "tmp"), 0);
    CHECK(instance_cbk_data.uniform_list[0].type, RDR_FLOAT);
    CHECK(instance_cbk_data.nb_attribs, 0);
  }

  sources[RDR_VERTEX_SHADER] = vs_source1;
  CHECK(rdr_material_program(sys, mtr, sources), RDR_NO_ERROR);
  if(!null_driver) {
    CHECK(instance_cbk_data.nb_uniforms, 0);
    CHECK(instance_cbk_data.nb_attribs, 2);
  }

  CHECK(rdr_mesh_data(sys, mesh, 3, attr2, sizeof(data), data), RDR_NO_ERROR);
  if(!null_driver) {
    CHECK(instance_cbk_data.nb_uniforms, 0);
    CHECK(instance_cbk_data.nb_attribs, 0);
  }

  CHECK(rdr_model_instance_transform(NULL, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_transform(sys, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_transform(NULL,inst, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_transform(sys, inst, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_transform(NULL, NULL, m44), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_transform(sys, NULL, m44), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_transform(NULL,inst, m44), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_transform(sys, inst, m44), RDR_NO_ERROR);
  CHECK(rdr_get_model_instance_transform(NULL,NULL,NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_transform(sys,NULL,NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_transform(NULL,inst,NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_transform(sys,inst,NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_transform(NULL,NULL, m), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_transform(sys,NULL, m), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_transform(NULL,inst, m), RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_transform(sys,inst, m), RDR_NO_ERROR);

  CHECK(rdr_model_instance_material_density(NULL, NULL, RDR_OPAQUE),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_material_density(sys, NULL, RDR_OPAQUE),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_material_density(sys, inst, RDR_OPAQUE),
        RDR_NO_ERROR);
  CHECK(rdr_get_model_instance_material_density(NULL, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_material_density(sys, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_material_density(NULL, inst, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_material_density(sys, inst, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_material_density(NULL, NULL, &mtr_density),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_material_density(sys, NULL, &mtr_density),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_material_density(NULL, inst, &mtr_density),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_material_density(sys, inst, &mtr_density),
        RDR_NO_ERROR);
  CHECK(mtr_density, RDR_OPAQUE);
  CHECK(rdr_model_instance_material_density(sys, inst, RDR_TRANSLUCENT),
        RDR_NO_ERROR);
  CHECK(rdr_get_model_instance_material_density(sys, inst, &mtr_density),
        RDR_NO_ERROR);
  CHECK(mtr_density, RDR_TRANSLUCENT);

  rast.fill_mode = RDR_WIREFRAME;
  rast.cull_mode = RDR_CULL_NONE;
  CHECK(rdr_model_instance_rasterizer(NULL, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_rasterizer(sys, NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_rasterizer(NULL, inst, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_rasterizer(sys, inst, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_rasterizer(NULL, NULL, &rast), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_rasterizer(sys, NULL, &rast), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_rasterizer(NULL, inst, &rast), RDR_INVALID_ARGUMENT);
  CHECK(rdr_model_instance_rasterizer(sys, inst, &rast), RDR_NO_ERROR);
  CHECK(rdr_model_instance_rasterizer(sys, inst, &rast), RDR_NO_ERROR);
  CHECK(rdr_get_model_instance_rasterizer(NULL, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_rasterizer(sys, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_rasterizer(NULL, inst, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_rasterizer(sys, inst, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_rasterizer(NULL, NULL, &rast),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_rasterizer(sys, NULL, &rast),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_rasterizer(NULL, inst, &rast),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_get_model_instance_rasterizer(sys, inst, &rast),RDR_NO_ERROR);
  CHECK(rast.fill_mode, RDR_WIREFRAME);
  CHECK(rast.cull_mode, RDR_CULL_NONE);
  rast.fill_mode = RDR_SOLID;
  rast.cull_mode = RDR_CULL_BACK;
  CHECK(rdr_model_instance_rasterizer(sys, inst, &rast), RDR_NO_ERROR);
  CHECK(rdr_get_model_instance_rasterizer(sys, inst, &rast), RDR_NO_ERROR);
  CHECK(rast.fill_mode, RDR_SOLID);
  CHECK(rast.cull_mode, RDR_CULL_BACK);

  CHECK(rdr_create_model_instance(sys, model, &inst1), RDR_NO_ERROR);

  sources[RDR_VERTEX_SHADER] = vs_source;
  CHECK(rdr_material_program(sys, mtr, sources), RDR_NO_ERROR);

  CHECK(rdr_free_model(sys, model), RDR_NO_ERROR);
  CHECK(rdr_free_mesh(sys, mesh), RDR_NO_ERROR);
  CHECK(rdr_free_material(sys, mtr), RDR_NO_ERROR);

  CHECK(rdr_detach_model_instance_callback(NULL, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_detach_model_instance_callback(sys, NULL, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_detach_model_instance_callback(NULL, inst, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_detach_model_instance_callback(sys, inst, NULL),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_detach_model_instance_callback(NULL, NULL, cbk),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_detach_model_instance_callback(sys, NULL, cbk),
        RDR_INVALID_ARGUMENT);
  CHECK(rdr_detach_model_instance_callback(NULL, inst, cbk),
        RDR_INVALID_ARGUMENT);
  #ifndef NDEBUG
  NCHECK(rdr_detach_model_instance_callback(sys, inst1, cbk),
        RDR_NO_ERROR);
  #endif
  CHECK(rdr_detach_model_instance_callback(sys, inst, cbk),
        RDR_NO_ERROR);

  CHECK(rdr_free_model_instance(NULL, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_free_model_instance(sys, NULL), RDR_INVALID_ARGUMENT);
  CHECK(rdr_free_model_instance(NULL, inst), RDR_INVALID_ARGUMENT);
  CHECK(rdr_free_model_instance(sys, inst), RDR_NO_ERROR);
  CHECK(rdr_free_model_instance(sys, inst1), RDR_NO_ERROR);

  CHECK(rdr_free_system(sys), RDR_NO_ERROR);
}

int
main(int argc, char** argv)
{
  const char* driver_name = NULL;
  FILE* file = NULL;
  int err = 0;

  /* Window manager data structures. */
  struct wm_device* device = NULL;
  struct wm_window* window = NULL;
  struct wm_window_desc win_desc = {
    .width = 800, .height = 600, .fullscreen = 0
  };

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    goto error;
  }
  driver_name = argv[1];

  file = fopen(driver_name, "r");
  if(!file) {
    fprintf(stderr, "Invalid driver %s\n", driver_name);
    goto error;
  }
  fclose(file);

  CHECK(wm_create_device(&device), 0);
  CHECK(wm_create_window(device, &win_desc, &window), 0);

  test_rdr_system(driver_name);
  test_rdr_mesh(driver_name);
  test_rdr_material(driver_name);
  test_rdr_model(driver_name);
  test_rdr_model_instance(driver_name);

exit:
  if(window)
    wm_free_window(device, window);
  if(device)
    wm_free_device(device);

  return err;

error:
  err = -1;
  goto exit;
}

