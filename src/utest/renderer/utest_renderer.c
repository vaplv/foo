#include "renderer/rdr_material.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_model.h"
#include "renderer/rdr_model_instance.h"
#include "renderer/rdr_system.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BAD_ARG RDR_INVALID_ARGUMENT
#define OK RDR_NO_ERROR
#define SZ sizeof

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

  CHECK(rdr_create_system(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_create_system(NULL, NULL, &sys), BAD_ARG);
  CHECK(rdr_create_system("__INVALID_DRIVER__", NULL, NULL), BAD_ARG);
  CHECK(rdr_create_system("__INVALID_DRIVER__", NULL, &sys), RDR_DRIVER_ERROR);
  CHECK(rdr_create_system(driver_name, NULL, &sys), OK);
  CHECK(rdr_system_ref_put(NULL), BAD_ARG);
  CHECK(rdr_system_ref_put(sys), OK);
}

static void
test_rdr_mesh(const char* driver_name)
{
  struct rdr_system* sys = NULL;
  struct rdr_mesh* mesh = NULL;
  const float vert1[] = { 0.f, 0.f, 0.f };
  const float vert2[] = { 0.f, 0.f, 0.f, 1.f, 1.f, 1.f };
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

  CHECK(rdr_create_system(driver_name, NULL, &sys), OK);

  CHECK(rdr_create_mesh(NULL, NULL), BAD_ARG);
  CHECK(rdr_create_mesh(NULL, &mesh), BAD_ARG);
  CHECK(rdr_create_mesh(sys, NULL), BAD_ARG);
  CHECK(rdr_create_mesh(sys, &mesh), OK);

  CHECK(rdr_mesh_data(NULL, 0, NULL, 0, NULL), BAD_ARG);
  CHECK(rdr_mesh_data(NULL, 1, NULL, 0, NULL), BAD_ARG);
  CHECK(rdr_mesh_data(NULL, 0, mesh_attr, 0, NULL), BAD_ARG);
  CHECK(rdr_mesh_data(NULL, 0, NULL, 0, NULL), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 0, mesh_attr, SZ(vert1), vert1), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 2, NULL, SZ(vert1), vert1), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 2, mesh_attr, 0, vert1), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 2, mesh_attr, SZ(vert1), NULL), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 2, NULL, SZ(vert1), NULL), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 1, mesh_attr, SZ(vert1), vert1), BAD_ARG);
  CHECK(rdr_mesh_data(NULL, 2, mesh_attr, SZ(vert1), vert1), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 1, mesh_attr2, SZ(vert1), vert1), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 1, mesh_attr3, SZ(vert1), vert1), BAD_ARG);
  CHECK(rdr_mesh_data(mesh, 0, mesh_attr, 0, vert1), OK);
  CHECK(rdr_mesh_data(mesh, 0, NULL, 0, NULL), OK);
  CHECK(rdr_mesh_data(mesh, 2, mesh_attr, SZ(vert1), vert1), OK);
  CHECK(rdr_mesh_data(mesh, 2, mesh_attr, SZ(vert2), vert2), OK);
  CHECK(rdr_mesh_data(mesh, 1, mesh_attr, SZ(vert2), vert2), OK);

  CHECK(rdr_mesh_indices(NULL, 3, NULL), BAD_ARG);
  CHECK(rdr_mesh_indices(mesh, 4, indices2), BAD_ARG);
  CHECK(rdr_mesh_indices(mesh, 3, indices1), OK);
  CHECK(rdr_mesh_indices(mesh, 0, indices1), OK);

  CHECK(rdr_mesh_ref_get(NULL), BAD_ARG);
  CHECK(rdr_mesh_ref_get(mesh), OK);
  CHECK(rdr_mesh_ref_put(NULL), BAD_ARG);
  CHECK(rdr_mesh_ref_put(mesh), OK);
  CHECK(rdr_mesh_ref_put(mesh), OK);
  CHECK(rdr_system_ref_put(sys), OK);
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

  memset(sources, 0, SZ(const char*) * RDR_NB_SHADER_USAGES);

  CHECK(rdr_create_system(driver_name, NULL, &sys), OK);

  CHECK(rdr_create_material(NULL, NULL), BAD_ARG);
  CHECK(rdr_create_material(sys, NULL), BAD_ARG);
  CHECK(rdr_create_material(NULL, &mtr), BAD_ARG);
  CHECK(rdr_create_material(sys, &mtr), OK);

  sources[RDR_VERTEX_SHADER] = bad_source;
  CHECK(rdr_material_program(NULL, NULL), BAD_ARG);
  CHECK(rdr_material_program(mtr, NULL), BAD_ARG);
  CHECK(rdr_material_program(NULL, sources), BAD_ARG);
  if(null_driver == false)
    CHECK(rdr_material_program(mtr, sources), RDR_DRIVER_ERROR);

  CHECK(rdr_get_material_log(NULL, NULL), BAD_ARG);
  CHECK(rdr_get_material_log(mtr, NULL), BAD_ARG);
  CHECK(rdr_get_material_log(NULL, &log), BAD_ARG);
  CHECK(rdr_get_material_log(mtr, &log), OK);

  sources[RDR_VERTEX_SHADER] = good_source;
  CHECK(rdr_material_program(mtr, sources), OK);

  CHECK(rdr_material_ref_get(NULL), BAD_ARG);
  CHECK(rdr_material_ref_get(mtr), OK);
  CHECK(rdr_material_ref_put(NULL), BAD_ARG);
  CHECK(rdr_material_ref_put(mtr), OK);
  CHECK(rdr_material_ref_put(mtr), OK);

  CHECK(rdr_system_ref_put(sys), OK);
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

  memset(sources, 0, SZ(const char*) * RDR_NB_SHADER_USAGES);
  sources[RDR_VERTEX_SHADER] = vs_source;

  CHECK(rdr_create_system(driver_name, NULL, &sys), OK);
  CHECK(rdr_create_mesh(sys, &mesh), OK);
  CHECK(rdr_mesh_data(mesh, 2, attr0, SZ(data), data), OK);
  CHECK(rdr_create_material(sys, &mtr), OK);
  CHECK(rdr_material_program(mtr, sources), OK);

  CHECK(rdr_create_model(NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_create_model(sys, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_create_model(NULL, mesh, NULL, NULL), BAD_ARG);
  CHECK(rdr_create_model(sys, mesh, NULL, NULL), BAD_ARG);

  CHECK(rdr_create_model(NULL, NULL, mtr, NULL), BAD_ARG);
  CHECK(rdr_create_model(sys, NULL, mtr, NULL), BAD_ARG);
  CHECK(rdr_create_model(NULL, mesh, mtr, NULL), BAD_ARG);
  CHECK(rdr_create_model(sys, mesh, mtr, NULL), BAD_ARG);

  CHECK(rdr_create_model(NULL, NULL, mtr, &model0), BAD_ARG);
  CHECK(rdr_create_model(sys, NULL, mtr, &model0), BAD_ARG);
  CHECK(rdr_create_model(NULL, mesh, mtr, &model0), BAD_ARG);
  CHECK(rdr_create_model(sys, mesh, mtr, &model0), OK);
  CHECK(rdr_create_model(sys, mesh, mtr, &model1), OK);

  CHECK(rdr_model_mesh(NULL, NULL), BAD_ARG);
  CHECK(rdr_model_mesh(model0, NULL), BAD_ARG);
  CHECK(rdr_model_mesh(NULL, mesh), BAD_ARG);
  CHECK(rdr_model_mesh(model0, mesh), OK);

  CHECK(rdr_mesh_data(mesh, 1, attr1, SZ(data), data), OK);

  CHECK(rdr_model_ref_get(NULL), BAD_ARG);
  CHECK(rdr_model_ref_get(model0), OK);

  CHECK(rdr_model_ref_put(NULL), BAD_ARG);
  CHECK(rdr_model_ref_put(model0), OK);
  CHECK(rdr_model_ref_put(model0), OK);
  CHECK(rdr_model_ref_put(model1), OK);

  CHECK(rdr_mesh_ref_put(mesh), OK);
  CHECK(rdr_material_ref_put(mtr), OK);
  CHECK(rdr_system_ref_put(sys), OK);
}

struct instance_cbk_data {
  struct rdr_model_instance_data* uniform_list;
  struct rdr_model_instance_data* attrib_list;
  size_t nb_uniforms;
  size_t nb_attribs;
};

static void
instance_cbk_func
  (struct rdr_model_instance* inst,
   void* data)
{
  struct instance_cbk_data* cbk_data = data;

  assert(cbk_data != NULL);

  if(cbk_data->uniform_list) {
    MEM_FREE(&mem_default_allocator, cbk_data->uniform_list);
    cbk_data->uniform_list = NULL;
    cbk_data->nb_uniforms = 0;
  }

  if(cbk_data->attrib_list) {
    MEM_FREE(&mem_default_allocator, cbk_data->attrib_list);
    cbk_data->attrib_list = NULL;
    cbk_data->nb_attribs = 0;
  }

  CHECK(rdr_get_model_instance_uniforms(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_uniforms(inst, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_uniforms
        (NULL, &cbk_data->nb_uniforms, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_uniforms(inst, NULL, NULL), BAD_ARG);
    CHECK(rdr_get_model_instance_uniforms
        (NULL, NULL, cbk_data->uniform_list), BAD_ARG);
    CHECK(rdr_get_model_instance_uniforms
        (inst, NULL, cbk_data->uniform_list), BAD_ARG);
  CHECK(rdr_get_model_instance_uniforms
        (NULL, &cbk_data->nb_uniforms, cbk_data->uniform_list), BAD_ARG);
  CHECK(rdr_get_model_instance_uniforms
        (inst, &cbk_data->nb_uniforms, NULL), OK);

  if(cbk_data->nb_uniforms > 0) {
    if(cbk_data->uniform_list)
      MEM_FREE(&mem_default_allocator, cbk_data->uniform_list);
    cbk_data->uniform_list = MEM_CALLOC
      (&mem_default_allocator,
       cbk_data->nb_uniforms,
       SZ(struct instance_cbk_data));
    assert(NULL != cbk_data->uniform_list);

    CHECK(rdr_get_model_instance_uniforms
          (inst, &cbk_data->nb_uniforms, cbk_data->uniform_list), OK);
  }

  CHECK(rdr_get_model_instance_attribs(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_attribs(inst, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_attribs
        (NULL, &cbk_data->nb_attribs, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_attribs(inst, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_attribs
        (NULL, NULL, cbk_data->attrib_list), BAD_ARG);
  CHECK(rdr_get_model_instance_attribs
        (inst, NULL, cbk_data->attrib_list), BAD_ARG);
  CHECK(rdr_get_model_instance_attribs
        (NULL, &cbk_data->nb_attribs, cbk_data->attrib_list), BAD_ARG);
  CHECK(rdr_get_model_instance_attribs
        (inst, &cbk_data->nb_attribs, NULL), OK);

  if(cbk_data->nb_attribs > 0) {
    if(cbk_data->attrib_list)
      MEM_FREE(&mem_default_allocator, cbk_data->attrib_list);
    cbk_data->attrib_list = MEM_CALLOC
      (&mem_default_allocator,
       cbk_data->nb_attribs,
       SZ(struct instance_cbk_data));
    assert(NULL != cbk_data->attrib_list);

    CHECK(rdr_get_model_instance_attribs
      (inst, &cbk_data->nb_attribs, cbk_data->attrib_list), OK);
  }
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
  enum rdr_material_density density;
  struct rdr_rasterizer_desc rast;
  bool null_driver = is_driver_null(driver_name);
  bool b = false;

  memset(&instance_cbk_data, 0, SZ(struct instance_cbk_data));
  memset(sources, 0, SZ(const char*) * RDR_NB_SHADER_USAGES);
  sources[RDR_VERTEX_SHADER] = vs_source;

  CHECK(rdr_create_system(driver_name, NULL, &sys), OK);
  CHECK(rdr_create_mesh(sys, &mesh), OK);
  CHECK(rdr_mesh_data(mesh, 2, attr0, SZ(data), data), OK);
  CHECK(rdr_create_material(sys, &mtr), OK);
  CHECK(rdr_material_program(mtr, sources), OK);
  CHECK(rdr_create_model(sys, mesh, mtr, &model), OK);

  CHECK(rdr_create_model_instance(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_create_model_instance(sys, NULL, NULL), BAD_ARG);
  CHECK(rdr_create_model_instance(NULL, model, NULL), BAD_ARG);
  CHECK(rdr_create_model_instance(sys, model, NULL), BAD_ARG);
  CHECK(rdr_create_model_instance(NULL, NULL, &inst), BAD_ARG);
  CHECK(rdr_create_model_instance(sys, NULL, &inst), BAD_ARG);
  CHECK(rdr_create_model_instance(NULL, model, &inst), BAD_ARG);
  CHECK(rdr_create_model_instance(sys, model, &inst), OK);

  CHECK(rdr_mesh_data(mesh, 1, attr1, SZ(data), data), OK);

  CHECK(rdr_attach_model_instance_callback(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_attach_model_instance_callback(inst, NULL, NULL), BAD_ARG);
  CHECK(rdr_attach_model_instance_callback
    (NULL, instance_cbk_func, NULL), BAD_ARG);
  CHECK(rdr_attach_model_instance_callback
    (inst, instance_cbk_func, NULL), OK);
  CHECK(rdr_detach_model_instance_callback
    (inst, instance_cbk_func, NULL), OK);
  CHECK(rdr_attach_model_instance_callback
    (NULL, NULL, &instance_cbk_data), BAD_ARG);
  CHECK(rdr_attach_model_instance_callback
    (inst, NULL, &instance_cbk_data), BAD_ARG);
  CHECK(rdr_attach_model_instance_callback
    (NULL, instance_cbk_func, &instance_cbk_data), BAD_ARG);
  CHECK(rdr_attach_model_instance_callback
    (inst, instance_cbk_func, &instance_cbk_data), OK);

  CHECK(rdr_mesh_data(mesh, 1, attr1, SZ(data), data), OK);
  if(!null_driver) {
    CHECK(instance_cbk_data.nb_uniforms, 1);
    CHECK(strcmp(instance_cbk_data.uniform_list[0].name, "tmp"), 0);
    CHECK(instance_cbk_data.uniform_list[0].type, RDR_FLOAT);
    CHECK(instance_cbk_data.nb_attribs, 1);
    CHECK(strcmp(instance_cbk_data.attrib_list[0].name, "rdr_color"), 0);
    CHECK(instance_cbk_data.attrib_list[0].type, RDR_FLOAT4);
  }

  CHECK(rdr_mesh_data(mesh, 2, attr0, SZ(data), data), OK);
  if(!null_driver) {
    CHECK(instance_cbk_data.nb_uniforms, 1);
    CHECK(strcmp(instance_cbk_data.uniform_list[0].name, "tmp"), 0);
    CHECK(instance_cbk_data.uniform_list[0].type, RDR_FLOAT);
    CHECK(instance_cbk_data.nb_attribs, 0);
    }

  sources[RDR_VERTEX_SHADER] = vs_source1;
  CHECK(rdr_material_program(mtr, sources), OK);
  if(!null_driver) {
    CHECK(instance_cbk_data.nb_uniforms, 0);
    CHECK(instance_cbk_data.nb_attribs, 2);
  }

  CHECK(rdr_mesh_data(mesh, 3, attr2, SZ(data), data), OK);
  if(!null_driver) {
    CHECK(instance_cbk_data.nb_uniforms, 0);
    CHECK(instance_cbk_data.nb_attribs, 0);
  }

  CHECK(rdr_model_instance_transform(NULL, NULL), BAD_ARG);
  CHECK(rdr_model_instance_transform(inst, NULL), BAD_ARG);
  CHECK(rdr_model_instance_transform(NULL, m44), BAD_ARG);
  CHECK(rdr_model_instance_transform(inst, m44), OK);
  CHECK(rdr_get_model_instance_transform(NULL,NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_transform(inst,NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_transform(NULL, m), BAD_ARG);
  CHECK(rdr_get_model_instance_transform(inst, m), OK);
  CHECK(m[0],  1.f); CHECK(m[1],  0.f); CHECK(m[2],  0.f); CHECK(m[3],  0.f);
  CHECK(m[4],  0.f); CHECK(m[5],  1.f); CHECK(m[6],  0.f); CHECK(m[7],  0.f);
  CHECK(m[8],  0.f); CHECK(m[9],  0.f); CHECK(m[10], 1.f); CHECK(m[11], 0.f);
  CHECK(m[12], 0.f); CHECK(m[13], 0.f); CHECK(m[14], 0.f); CHECK(m[15], 1.f);

  CHECK(rdr_translate_model_instance(NULL, false, NULL), BAD_ARG);
  CHECK(rdr_translate_model_instance(inst, false, NULL), BAD_ARG);
  CHECK(rdr_translate_model_instance
    (NULL, false, (float[]){1.f, 5.f, 3.f}), BAD_ARG);
  CHECK(rdr_translate_model_instance
    (inst, false, (float[]){1.f, 5.f, 3.f}), OK);
  CHECK(rdr_get_model_instance_transform(inst, m), OK);
  CHECK(m[0],  1.f); CHECK(m[1],  0.f); CHECK(m[2],  0.f); CHECK(m[3],  0.f);
  CHECK(m[4],  0.f); CHECK(m[5],  1.f); CHECK(m[6],  0.f); CHECK(m[7],  0.f);
  CHECK(m[8],  0.f); CHECK(m[9],  0.f); CHECK(m[10], 1.f); CHECK(m[11], 0.f);
  CHECK(m[12], 1.f); CHECK(m[13], 5.f); CHECK(m[14], 3.f); CHECK(m[15], 1.f);
  CHECK(rdr_translate_model_instance
    (inst, false, (float[]){-4.f, 2.1f, 1.f}), OK);
  CHECK(rdr_get_model_instance_transform(inst, m), OK);
  CHECK(m[0],  1.f); CHECK(m[1],  0.f); CHECK(m[2],  0.f); CHECK(m[3],  0.f);
  CHECK(m[4],  0.f); CHECK(m[5],  1.f); CHECK(m[6],  0.f); CHECK(m[7],  0.f);
  CHECK(m[8],  0.f); CHECK(m[9],  0.f); CHECK(m[10], 1.f); CHECK(m[11], 0.f);
  CHECK(m[12],-3.f); CHECK(m[13],7.1f); CHECK(m[14], 4.f); CHECK(m[15], 1.f);
  
  CHECK(rdr_model_instance_material_density(NULL, RDR_OPAQUE), BAD_ARG);
  CHECK(rdr_model_instance_material_density(inst, RDR_OPAQUE), OK);
  CHECK(rdr_get_model_instance_material_density(NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_material_density(inst, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_material_density(NULL, &density), BAD_ARG);
  CHECK(rdr_get_model_instance_material_density(inst, &density), OK);
  CHECK(density, RDR_OPAQUE);
  CHECK(rdr_model_instance_material_density(inst, RDR_TRANSLUCENT), OK);
  CHECK(rdr_get_model_instance_material_density(inst, &density), OK);
  CHECK(density, RDR_TRANSLUCENT);

  rast.fill_mode = RDR_WIREFRAME;
  rast.cull_mode = RDR_CULL_NONE;
  CHECK(rdr_model_instance_rasterizer(NULL, NULL), BAD_ARG);
  CHECK(rdr_model_instance_rasterizer(inst, NULL), BAD_ARG);
  CHECK(rdr_model_instance_rasterizer(NULL, &rast), BAD_ARG);
  CHECK(rdr_model_instance_rasterizer(inst, &rast), OK);
  CHECK(rdr_model_instance_rasterizer(inst, &rast), OK);
  CHECK(rdr_get_model_instance_rasterizer(NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_rasterizer(inst, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_rasterizer(NULL, &rast), BAD_ARG);
  CHECK(rdr_get_model_instance_rasterizer(inst, &rast),OK);
  CHECK(rast.fill_mode, RDR_WIREFRAME);
  CHECK(rast.cull_mode, RDR_CULL_NONE);
  rast.fill_mode = RDR_SOLID;
  rast.cull_mode = RDR_CULL_BACK;
  CHECK(rdr_model_instance_rasterizer(inst, &rast), OK);
  CHECK(rdr_get_model_instance_rasterizer(inst, &rast), OK);
  CHECK(rast.fill_mode, RDR_SOLID);
  CHECK(rast.cull_mode, RDR_CULL_BACK);

  CHECK(rdr_create_model_instance(sys, model, &inst1), OK);

  sources[RDR_VERTEX_SHADER] = vs_source;
  CHECK(rdr_material_program(mtr, sources), OK);

  CHECK(rdr_model_ref_put(model), OK);
  CHECK(rdr_mesh_ref_put(mesh), OK);
  CHECK(rdr_material_ref_put(mtr), OK);

  CHECK(rdr_is_model_instance_callback_attached
    (NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (inst, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (NULL, instance_cbk_func, NULL, NULL), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (inst, instance_cbk_func, NULL, NULL), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (NULL, NULL, &instance_cbk_data, NULL), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (inst, NULL, &instance_cbk_data, NULL), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (NULL, instance_cbk_func, &instance_cbk_data, NULL), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (inst, instance_cbk_func, &instance_cbk_data, NULL), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (NULL, NULL, NULL, &b), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (inst, NULL, NULL, &b), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (NULL, instance_cbk_func, NULL, &b), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (inst, instance_cbk_func, NULL, &b), OK);
  CHECK(b, false);
  CHECK(rdr_is_model_instance_callback_attached
    (NULL, NULL, &instance_cbk_data, &b), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (inst, NULL, &instance_cbk_data, &b), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (NULL, instance_cbk_func, &instance_cbk_data, &b), BAD_ARG);
  CHECK(rdr_is_model_instance_callback_attached
    (inst, instance_cbk_func, &instance_cbk_data, &b), OK);
  CHECK(b, true);

  CHECK(rdr_detach_model_instance_callback(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_detach_model_instance_callback(inst, NULL, NULL), BAD_ARG);
  CHECK(rdr_detach_model_instance_callback
    (NULL, instance_cbk_func, NULL), BAD_ARG);
  CHECK(rdr_detach_model_instance_callback
    (inst, instance_cbk_func, NULL), BAD_ARG);
  CHECK(rdr_detach_model_instance_callback
    (NULL, NULL, &instance_cbk_data), BAD_ARG);
  CHECK(rdr_detach_model_instance_callback
    (inst, NULL, &instance_cbk_data), BAD_ARG);
  CHECK(rdr_detach_model_instance_callback
    (NULL, instance_cbk_func, &instance_cbk_data), BAD_ARG);
  CHECK(rdr_detach_model_instance_callback
    (inst, instance_cbk_func, &instance_cbk_data), OK);

  if(instance_cbk_data.uniform_list)
    MEM_FREE(&mem_default_allocator, instance_cbk_data.uniform_list);
  if(instance_cbk_data.attrib_list)
    MEM_FREE(&mem_default_allocator, instance_cbk_data.attrib_list);

  CHECK(rdr_model_instance_ref_get(NULL), BAD_ARG);
  CHECK(rdr_model_instance_ref_get(inst), OK);

  CHECK(rdr_model_instance_ref_put(NULL), BAD_ARG);
  CHECK(rdr_model_instance_ref_put(inst), OK);
  CHECK(rdr_model_instance_ref_put(inst), OK);
  CHECK(rdr_model_instance_ref_put(inst1), OK);

  CHECK(rdr_system_ref_put(sys), OK);
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

  CHECK(wm_create_device(NULL, &device), WM_NO_ERROR);
  CHECK(wm_create_window(device, &win_desc, &window), WM_NO_ERROR);

  test_rdr_system(driver_name);
  test_rdr_mesh(driver_name);
  test_rdr_material(driver_name);
  test_rdr_model(driver_name);
  test_rdr_model_instance(driver_name);

exit:
  if(window)
    CHECK(wm_window_ref_put(window), WM_NO_ERROR);
  if(device)
    CHECK(wm_device_ref_put(device), WM_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return err;

error:
  err = -1;
  goto exit;
}

