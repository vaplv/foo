#include "maths/simd/aosf33.h"
#include "maths/simd/aosf44.h"
#include "renderer/rdr_material.h"
#include "renderer/rdr_mesh.h"
#include "renderer/rdr_model.h"
#include "renderer/rdr_model_instance.h"
#include "renderer/rdr_system.h"
#include "sys/mem_allocator.h"
#include "utest/utest.h"
#include "window_manager/wm_device.h"
#include "window_manager/wm_window.h"
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define BAD_ARG RDR_INVALID_ARGUMENT
#define OK RDR_NO_ERROR
#define SZ sizeof
#define CHECK_EPS(a, b) \
  CHECK(fabsf((a) - (b)) < 1.e-6f, true)

#define EPS 1.e-8f
#define CHECK_TRANSFORM(inst, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
  do { \
    float m__[16]; \
    CHECK(rdr_get_model_instance_transform(inst, m__), OK); \
    CHECK(fabsf(m__[0] - a) < EPS, true); \
    CHECK(fabsf(m__[1] - b) < EPS, true); \
    CHECK(fabsf(m__[2] - c) < EPS, true); \
    CHECK(fabsf(m__[3] - d) < EPS, true); \
    CHECK(fabsf(m__[4] - e) < EPS, true); \
    CHECK(fabsf(m__[5] - f) < EPS, true); \
    CHECK(fabsf(m__[6] - g) < EPS, true); \
    CHECK(fabsf(m__[7] - h) < EPS, true); \
    CHECK(fabsf(m__[8] - i) < EPS, true); \
    CHECK(fabsf(m__[9] - j) < EPS, true); \
    CHECK(fabsf(m__[10] - k) < EPS, true); \
    CHECK(fabsf(m__[11] - l) < EPS, true); \
    CHECK(fabsf(m__[12] - m) < EPS, true); \
    CHECK(fabsf(m__[13] - n) < EPS, true); \
    CHECK(fabsf(m__[14] - o) < EPS, true); \
    CHECK(fabsf(m__[15] - p) < EPS, true); \
  } while(0)

struct instance_cbk_data {
  struct rdr_model_instance_data* uniform_list;
  struct rdr_model_instance_data* attrib_list;
  size_t nb_uniforms;
  size_t nb_attribs;
};

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
test_bounding(struct rdr_system* sys)
{
  const float data[] = {
    0.f, 0.f, 0.f,
    1.f, 0.f, 0.f,
    1.f, 1.f, 0.f,
    0.f, 1.f, 0.f,
    0.f, 0.f, 1.f,
    1.f, 0.f, 1.f,
    1.f, 1.f, 1.f,
    0.f, 1.f, 1.f
  };
  const struct rdr_mesh_attrib attr[] = {
    { .usage = RDR_ATTRIB_POSITION, .type = RDR_FLOAT3 },
  };
  struct aosf33 f33;
  struct aosf44 f44;
  vf4_t vec0;
  vf4_t vec1;
  vf4_t vec2;
  vf4_t vec3;
  struct rdr_material* mtr = NULL;
  struct rdr_mesh* mesh = NULL;
  struct rdr_model* mdl = NULL;
  struct rdr_model_instance* inst = NULL;
  const char* sources[RDR_NB_SHADER_USAGES] = {
    [RDR_GEOMETRY_SHADER] = NULL,
    [RDR_FRAGMENT_SHADER] = NULL,
    [RDR_VERTEX_SHADER] =
      "#version 330\n"
      "void main()\n"
      "{\n"
      " gl_Position = vec4(0.f);\n"
      "}\n"
  };
  float pos[3];
  float extx[3];
  float exty[3];
  float extz[3];
  float min_bound[3];
  float max_bound[3];
  ALIGN(16) float array[4];
  size_t i = 0;

  NCHECK(sys, NULL);
  CHECK(rdr_create_mesh(sys, &mesh), OK);
  CHECK(rdr_mesh_data(mesh, 1, attr, SZ(data), data), OK);
  CHECK(rdr_create_material(sys, &mtr), OK);
  CHECK(rdr_material_program(mtr, sources), OK);
  CHECK(rdr_create_model(sys, mesh, mtr, &mdl), OK);
  CHECK(rdr_create_model_instance(sys, mdl, &inst), OK);

  CHECK(rdr_get_model_instance_aabb(NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_aabb(inst, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_aabb(NULL, min_bound, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_aabb(inst, min_bound, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_aabb(NULL, NULL, max_bound), BAD_ARG);
  CHECK(rdr_get_model_instance_aabb(inst, NULL, max_bound), BAD_ARG);
  CHECK(rdr_get_model_instance_aabb(NULL, min_bound, max_bound), BAD_ARG);
  CHECK(rdr_get_model_instance_aabb(inst, min_bound, max_bound), OK);
  CHECK(min_bound[0], 0.f);
  CHECK(min_bound[1], 0.f);
  CHECK(min_bound[2], 0.f);
  CHECK(max_bound[0], 1.f);
  CHECK(max_bound[1], 1.f);
  CHECK(max_bound[2], 1.f);

  CHECK(rdr_get_model_instance_obb(NULL, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, pos, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, pos, NULL, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, NULL, extx, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, NULL, extx, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, pos, extx, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, pos, extx, NULL, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, NULL, NULL, exty, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, NULL, NULL, exty, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, pos, NULL, exty, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, pos, NULL, exty, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, NULL, extx, exty, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, NULL, extx, exty, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, pos, extx, exty, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, pos, extx, exty, NULL), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, NULL, NULL, NULL, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, NULL, NULL, NULL, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, pos, NULL, NULL, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, pos, NULL, NULL, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, NULL, extx, NULL, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, NULL, extx, NULL, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, pos, extx, NULL, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, pos, extx, NULL, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, NULL, NULL, exty, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, NULL, NULL, exty, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, pos, NULL, exty, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, pos, NULL, exty, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, NULL, extx, exty, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, NULL, extx, exty, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(NULL, pos, extx, exty, extz), BAD_ARG);
  CHECK(rdr_get_model_instance_obb(inst, pos, extx, exty, extz), OK);
  CHECK(pos[0], 0.5f);
  CHECK(pos[1], 0.5f);
  CHECK(pos[2], 0.5f);
  CHECK(extx[0], 0.5f); CHECK(extx[1], 0.f); CHECK(extx[2], 0.f);
  CHECK(exty[0], 0.f); CHECK(exty[1], 0.5f); CHECK(exty[2], 0.f);
  CHECK(extz[0], 0.f); CHECK(extz[1], 0.f); CHECK(extz[2], 0.5f);

  aosf33_rotation(&f33, 0.785f, 0.11f, 0.87f);
  aosf44_set(&f44, f33.c0, f33.c1, f33.c2, vf4_set(15.878f, -5.68f, 0.25f, 1.f));

  CHECK(rdr_transform_model_instances(&inst, 1, true, &f44), OK);
  CHECK(rdr_get_model_instance_aabb(inst, min_bound, max_bound), OK);

  vec0 = vf4_set1(FLT_MAX);
  vec1 = vf4_set1(-FLT_MAX);
  for(i = 0; i < 8; ++i)  {
    const vf4_t tmp = aosf44_mulf4
      (&f44, vf4_set(data[i*3], data[i*3+1], data[i*3+2], 1.f));
    vf4_store(array, tmp);
    vec0 = vf4_min(vec0, tmp);
    vec1 = vf4_max(vec1, tmp);
  }
  vf4_store(array, vec0);
  CHECK_EPS(array[0], min_bound[0]);
  CHECK_EPS(array[1], min_bound[1]);
  CHECK_EPS(array[2], min_bound[2]);
  vf4_store(array, vec1);
  CHECK_EPS(array[0], max_bound[0]);
  CHECK_EPS(array[1], max_bound[1]);
  CHECK_EPS(array[2], max_bound[2]);

  vec0 = aosf44_mulf4(&f44, vf4_set(pos[0], pos[1], pos[2], 1.f));
  vec1 = aosf44_mulf4(&f44, vf4_set(extx[0], extx[1], extx[2], 0.f));
  vec2 = aosf44_mulf4(&f44, vf4_set(exty[0], exty[1], exty[2], 0.f));
  vec3 = aosf44_mulf4(&f44, vf4_set(extz[0], extz[1], extz[2], 0.f));
  CHECK(rdr_get_model_instance_obb(inst, pos, extx, exty, extz), OK);
  vf4_store(array, vec0);
  CHECK_EPS(array[0], pos[0]);
  CHECK_EPS(array[1], pos[1]);
  CHECK_EPS(array[2], pos[2]);
  vf4_store(array, vec1);
  CHECK_EPS(array[0], extx[0]);
  CHECK_EPS(array[1], extx[1]);
  CHECK_EPS(array[2], extx[2]);
  vf4_store(array, vec2);
  CHECK_EPS(array[0], exty[0]);
  CHECK_EPS(array[1], exty[1]);
  CHECK_EPS(array[2], exty[2]);
  vf4_store(array, vec3);
  CHECK_EPS(array[0], extz[0]);
  CHECK_EPS(array[1], extz[1]);
  CHECK_EPS(array[2], extz[2]);

  CHECK(rdr_model_ref_put(mdl), OK);
  CHECK(rdr_model_instance_ref_put(inst), OK);
  CHECK(rdr_material_ref_put(mtr), OK);
  CHECK(rdr_mesh_ref_put(mesh), OK);
}

int
main(int argc, char** argv)
{
  const char* driver_name = NULL;
  FILE* file = NULL;
  struct rdr_system* sys = NULL;
  struct wm_device* device = NULL;
  struct wm_window* window = NULL;
  struct wm_window_desc win_desc = {
    .width = 800, .height = 600, .fullscreen = 0
  };
  struct instance_cbk_data instance_cbk_data;
  struct rdr_material* mtr = NULL;
  struct rdr_mesh* mesh = NULL;
  struct rdr_model* model = NULL;
  struct rdr_model_instance* inst = NULL;
  struct rdr_model_instance* inst1 = NULL;
  const char* vs_source =
    "#version 330\n"
    "uniform mat4x4 rdr_modelviewproj;\n"
    "uniform float tmp;\n"
    "in vec4 rdr_position;\n"
    "in vec4 rdr_color; \n"
    "smooth out vec4 color; \n"
    "void main()\n"
    "{\n"
      "vec4 v = tmp * rdr_position;\n"
      "gl_Position = rdr_modelviewproj * v;\n"
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
  struct aosf44 f44;
  struct aosf44 f44a;
  struct aosf33 f33;
  struct aosf33 f33a;
  ALIGN(16) float tmp[16];
  enum rdr_material_density density;
  struct rdr_rasterizer_desc rast;
  bool null_driver = false;
  bool b = false;

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    return -1;
  }
  driver_name = argv[1];
  file = fopen(driver_name, "r");
  if(!file) {
    fprintf(stderr, "Invalid driver %s\n", driver_name);
    return -1;
  }
  fclose(file);

  null_driver = is_driver_null(driver_name);
  memset(&instance_cbk_data, 0, SZ(struct instance_cbk_data));
  memset(sources, 0, SZ(const char*) * RDR_NB_SHADER_USAGES);
  sources[RDR_VERTEX_SHADER] = vs_source;

  CHECK(wm_create_device(NULL, &device), WM_NO_ERROR);
  CHECK(wm_create_window(device, &win_desc, &window), WM_NO_ERROR);

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
  CHECK(rdr_get_model_instance_transform(NULL, tmp), BAD_ARG);
  CHECK(rdr_get_model_instance_transform(inst, tmp), OK);
  CHECK_TRANSFORM(inst,
     1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     0.f, 0.f, 0.f, 1.f);


  CHECK(rdr_translate_model_instances(NULL, 1, false, NULL), BAD_ARG);
  CHECK(rdr_translate_model_instances(&inst, 1, false, NULL), BAD_ARG);
  CHECK(rdr_translate_model_instances
    (NULL, 1, false, (float[]){1.f, 5.f, 3.f}), BAD_ARG);
  CHECK(rdr_translate_model_instances
    (&inst, 1, false, (float[]){1.f, 5.f, 3.f}), OK);
  CHECK_TRANSFORM(inst,
     1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     1.f, 5.f, 3.f, 1.f);
  CHECK(rdr_translate_model_instances
    (&inst, 1, false, (float[]){-4.f, 2.1f, 1.f}), OK);
  CHECK_TRANSFORM(inst,
     1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     -3.f, 7.1f, 4.f, 1.f);
  CHECK(rdr_translate_model_instances
    (&inst, 0, false, (float[]){-4.f, 2.1f, 1.f}), OK);
  CHECK_TRANSFORM(inst,
     1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     -3.f, 7.1f, 4.f, 1.f);
  CHECK(rdr_translate_model_instances
    (NULL, 0, false, (float[]){-4.f, 2.1f, 1.f}), OK);
  CHECK_TRANSFORM(inst,
     1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     -3.f, 7.1f, 4.f, 1.f);

  CHECK(rdr_move_model_instances(NULL, 0, NULL), BAD_ARG);
  CHECK(rdr_move_model_instances(&inst, 0, NULL), BAD_ARG);
  CHECK(rdr_move_model_instances(NULL, 1, NULL), BAD_ARG);
  CHECK(rdr_move_model_instances(&inst, 1, NULL), BAD_ARG);
  CHECK(rdr_move_model_instances(NULL, 0, (float[]){0.f, 0.f, 0.f}), OK);
  CHECK_TRANSFORM(inst,
     1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     -3.f, 7.1f, 4.f, 1.f);
  CHECK(rdr_move_model_instances(&inst, 0, (float[]){0.f, 0.f, 0.f}), OK);
  CHECK_TRANSFORM(inst,
     1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     -3.f, 7.1f, 4.f, 1.f);
  CHECK(rdr_move_model_instances(NULL, 1, (float[]){0.f, 0.f, 0.f}), BAD_ARG);
  CHECK(rdr_move_model_instances(&inst, 1, (float[]){0.f, 0.f, 0.f}), OK);
  CHECK_TRANSFORM(inst,
     1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     0.f, 0.f, 0.f, 1.f);
  CHECK(rdr_move_model_instances(&inst, 1, (float[]){5.f, 3.1f, -1.8f}), OK);
  CHECK_TRANSFORM(inst,
     1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     5.f, 3.1f, -1.8f, 1.f);
  CHECK(rdr_move_model_instances(&inst, 1, (float[]){0.f, 0.f, 0.f}), OK);

  CHECK(rdr_rotate_model_instances(NULL, 0, false, NULL), BAD_ARG);
  CHECK(rdr_rotate_model_instances(&inst, 0, false, NULL), BAD_ARG);
  CHECK(rdr_rotate_model_instances
    (NULL, 0, false, (float[]){0.785f, 0.f, 0.f}), OK);
  CHECK_TRANSFORM(inst,
     1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     0.f, 0.f, 0.f, 1.f);
  CHECK(rdr_rotate_model_instances
    (&inst, 0, false, (float[]){0.785f, 0.f, 0.f}), OK);
  CHECK_TRANSFORM(inst,
     1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     0.f, 0.f, 0.f, 1.f);
  CHECK(rdr_rotate_model_instances
    (&inst, 1, false, (float[]){0.785f, 0.f, 0.f}), OK);
  aosf33_rotation(&f33, 0.785f, 0.f, 0.f);
  f44.c0 = f33.c0;
  f44.c1 = f33.c1;
  f44.c2 = f33.c2;
  f44.c3 = vf4_set(0.f, 0.f, 0.f, 1.f);
  CHECK(rdr_get_model_instance_transform(inst, tmp), OK);
  aosf44_store(tmp, &f44);
    CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);

  CHECK(rdr_translate_model_instances
    (&inst, 1, false, (float[]){-4.f, 2.1f, 1.f}), OK);
  aosf44_mulf44
    (&f44,
     (struct aosf44[]) {{
      vf4_set(1.f, 0.f, 0.f, 0.f),
      vf4_set(0.f, 1.f, 0.f, 0.f),
      vf4_set(0.f, 0.f, 1.f, 0.f),
      vf4_set(-4.f, 2.1f, 1.f, 1.f)}},
    &f44);
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(rdr_translate_model_instances
    (&inst, 1, true, (float[]){-4.f, 2.1f, 1.f}), OK);
  aosf44_mulf44
    (&f44,
     &f44,
     (struct aosf44[]) {{
      vf4_set(1.f, 0.f, 0.f, 0.f),
      vf4_set(0.f, 1.f, 0.f, 0.f),
      vf4_set(0.f, 0.f, 1.f, 0.f),
      vf4_set(-4.f, 2.1f, 1.f, 1.f)}});
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(rdr_rotate_model_instances
    (&inst, 1, true, (float[]){0.785f, 0.11f, 0.87f}), OK);
  aosf33_rotation(&f33, 0.785f, 0.11f, 0.87f);
  aosf33_mulf33(&f33, (struct aosf33[]) {{f44.c0, f44.c1, f44.c2}}, &f33);
  f44.c0 = f33.c0;
  f44.c1 = f33.c1;
  f44.c2 = f33.c2;
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(rdr_rotate_model_instances
    (&inst, 1, false, (float[]){0.785f, 0.11f, 0.87f}), OK);
  aosf33_rotation(&f33, 0.785f, 0.11f, 0.87f);
  aosf44_mulf44
    (&f44,
     (struct aosf44[]) {{f33.c0, f33.c1, f33.c2, vf4_set(0.f, 0.f, 0.f, 1.f)}},
     &f44);
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);

  CHECK(rdr_scale_model_instances(NULL, 0, false, NULL), BAD_ARG);
  CHECK(rdr_scale_model_instances(&inst, 0, false, NULL), BAD_ARG);
  CHECK(rdr_scale_model_instances
    (NULL, 0, true, (float[]){5.f, 4.f, 2.1f}), OK);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(rdr_scale_model_instances
    (&inst, 0, true, (float[]){5.f, 4.f, 2.1f}), OK);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(rdr_scale_model_instances
    (&inst, 1, true, (float[]){5.f, 4.f, 2.1f}), OK);
  aosf33_mulf33
    (&f33,
     (struct aosf33[]) {{f44.c0, f44.c1, f44.c2}},
     (struct aosf33[]) {{
      vf4_set(5.f, 0.f, 0.f, 0.f),
      vf4_set(0.f, 4.f, 0.f, 0.f),
      vf4_set(0.f, 0.f, 2.1f, 0.f)}});
  f44.c0 = f33.c0;
  f44.c1 = f33.c1;
  f44.c2 = f33.c2;
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(rdr_scale_model_instances
    (&inst, 1, false, (float[]){7.1f, 1.98f, 0.5f}), OK);
  aosf44_mulf44
    (&f44,
     (struct aosf44[]) {{
      vf4_set(7.1f, 0.f, 0.f, 0.f),
      vf4_set(0.f, 1.98f, 0.f, 0.f),
      vf4_set(0.f, 0.f, 0.5f, 0.f),
      vf4_set(0.f, 0.f, 0.f, 1.f)}},
     &f44);
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);

  CHECK(rdr_transform_model_instances(NULL, 0, true, NULL), BAD_ARG);
  CHECK(rdr_transform_model_instances(&inst, 0, true, NULL), BAD_ARG);
  CHECK(rdr_transform_model_instances(NULL, 0, true, &f44), OK);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(rdr_transform_model_instances(&inst, 0, true, &f44), OK);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(rdr_transform_model_instances
    (&inst, 1, true,
     (struct aosf44[]){{
      vf4_set(45.f, 4.f, 7.8f, 0.f),
      vf4_set(5.1f, 2.f, -6.f, 0.f),
      vf4_set(0.5f, -50.f, 899.f, 0.f),
      vf4_set(4.5f, 0.01f, -989.f, 1.f)}}), OK);
  aosf44_mulf44
    (&f44, &f44,
     (struct aosf44[]){{
     vf4_set(45.f, 4.f, 7.8f, 0.f),
     vf4_set(5.1f, 2.f, -6.f, 0.f),
     vf4_set(0.5f, -50.f, 899.f, 0.f),
     vf4_set(4.5f, 0.01f, -989.f, 1.f)}});
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(rdr_transform_model_instances
    (&inst, 1, false,
     (struct aosf44[]){{
      vf4_set(45.f, 4.f, 7.8f, 0.f),
      vf4_set(5.1f, 2.f, -6.f, 0.f),
      vf4_set(0.5f, -50.f, 899.f, 0.f),
      vf4_set(4.5f, 0.01f, -989.f, 1.f)}}), OK);
  aosf44_mulf44
    (&f44,
     (struct aosf44[]){{
     vf4_set(45.f, 4.f, 7.8f, 0.f),
     vf4_set(5.1f, 2.f, -6.f, 0.f),
     vf4_set(0.5f, -50.f, 899.f, 0.f),
     vf4_set(4.5f, 0.01f, -989.f, 1.f)}},
     &f44);
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);

  CHECK(rdr_create_model_instance(sys, model, &inst1), OK);
  CHECK(rdr_get_model_instance_transform(inst1, tmp), OK);
  aosf44_set
    (&f44a,
     vf4_set(tmp[0], tmp[1], tmp[2], tmp[3]),
     vf4_set(tmp[4], tmp[5], tmp[6], tmp[7]),
     vf4_set(tmp[8], tmp[9], tmp[10], tmp[11]),
     vf4_set(tmp[12], tmp[13], tmp[14], tmp[15]));

  CHECK(rdr_translate_model_instances
    ((struct rdr_model_instance*[]){inst, inst1}, 2,
     false, (float[]){5.f, 1.f, 4.f}), OK);
  aosf44_mulf44
    (&f44,
     (struct aosf44[]){{
      vf4_set(1.f, 0.f, 0.f, 0.f),
      vf4_set(0.f, 1.f, 0.f, 0.f),
      vf4_set(0.f, 0.f, 1.f, 0.f),
      vf4_set(5.f, 1.f, 4.f, 1.f)}},
     &f44);
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  aosf44_mulf44
    (&f44a,
     (struct aosf44[]){{
      vf4_set(1.f, 0.f, 0.f, 0.f),
      vf4_set(0.f, 1.f, 0.f, 0.f),
      vf4_set(0.f, 0.f, 1.f, 0.f),
      vf4_set(5.f, 1.f, 4.f, 1.f)}},
     &f44a);
  aosf44_store(tmp, &f44a);
  CHECK_TRANSFORM(inst1,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(rdr_translate_model_instances
    ((struct rdr_model_instance*[]){inst, inst1}, 2,
     true, (float[]){5.f, 1.f, 4.f}), OK);
  aosf44_mulf44
    (&f44,
     &f44,
     (struct aosf44[]){{
      vf4_set(1.f, 0.f, 0.f, 0.f),
      vf4_set(0.f, 1.f, 0.f, 0.f),
      vf4_set(0.f, 0.f, 1.f, 0.f),
      vf4_set(5.f, 1.f, 4.f, 1.f)}});
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  aosf44_mulf44
    (&f44a,
     &f44a,
     (struct aosf44[]){{
      vf4_set(1.f, 0.f, 0.f, 0.f),
      vf4_set(0.f, 1.f, 0.f, 0.f),
      vf4_set(0.f, 0.f, 1.f, 0.f),
      vf4_set(5.f, 1.f, 4.f, 1.f)}});
  aosf44_store(tmp, &f44a);
  CHECK_TRANSFORM(inst1,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);

  CHECK(rdr_rotate_model_instances
    ((struct rdr_model_instance*[]){inst, inst1}, 2,
     false, (float[]){0.785f, 0.12f, 0.08f}), OK);
  aosf33_rotation(&f33, 0.785f, 0.12f, 0.08f);
  aosf44_mulf44
    (&f44,
     (struct aosf44[]) {{f33.c0, f33.c1, f33.c2, vf4_set(0.f, 0.f, 0.f, 1.f)}},
     &f44);
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  aosf44_mulf44
    (&f44a,
     (struct aosf44[]) {{f33.c0, f33.c1, f33.c2, vf4_set(0.f, 0.f, 0.f, 1.f)}},
     &f44a);
  aosf44_store(tmp, &f44a);
  CHECK_TRANSFORM(inst1,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(rdr_rotate_model_instances
    ((struct rdr_model_instance*[]){inst, inst1}, 2,
     true, (float[]){0.785f, 0.12f, 0.08f}), OK);
  aosf33_rotation(&f33, 0.785f, 0.12f, 0.08f);
  aosf33_mulf33(&f33a, (struct aosf33[]) {{f44.c0, f44.c1, f44.c2}}, &f33);
  f44.c0 = f33a.c0;
  f44.c1 = f33a.c1;
  f44.c2 = f33a.c2;
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  aosf33_mulf33(&f33a, (struct aosf33[]) {{f44a.c0, f44a.c1, f44a.c2}}, &f33);
  f44a.c0 = f33a.c0;
  f44a.c1 = f33a.c1;
  f44a.c2 = f33a.c2;
  aosf44_store(tmp, &f44a);
  CHECK_TRANSFORM(inst1,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);

  CHECK(rdr_scale_model_instances
    ((struct rdr_model_instance*[]){inst, inst1}, 2,
     false, (float[]){0.5f, 0.789f, -2.78f}), OK);
  aosf44_mulf44
    (&f44,
     (struct aosf44[]) {{
      vf4_set(0.5f, 0.f, 0.f, 0.f),
      vf4_set(0.f, 0.789, 0.f, 0.f),
      vf4_set(0.f, 0.f, -2.78f, 0.f),
      vf4_set(0.f, 0.f, 0.f, 1.f)}},
     &f44);
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  aosf44_mulf44
    (&f44a,
     (struct aosf44[]) {{
      vf4_set(0.5f, 0.f, 0.f, 0.f),
      vf4_set(0.f, 0.789, 0.f, 0.f),
      vf4_set(0.f, 0.f, -2.78f, 0.f),
      vf4_set(0.f, 0.f, 0.f, 1.f)}},
     &f44a);
  aosf44_store(tmp, &f44a);
  CHECK_TRANSFORM(inst1,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(rdr_scale_model_instances
    ((struct rdr_model_instance*[]){inst, inst1}, 2,
     true, (float[]){0.5f, 0.789f, -2.78f}), OK);
  aosf33_mulf33
    (&f33,
     (struct aosf33[]) {{f44.c0, f44.c1, f44.c2}},
     (struct aosf33[]) {{
      vf4_set(0.5f, 0.f, 0.f, 0.f),
      vf4_set(0.f, 0.789f, 0.f, 0.f),
      vf4_set(0.f, 0.f, -2.78f, 0.f)}});
  f44.c0 = f33.c0;
  f44.c1 = f33.c1;
  f44.c2 = f33.c2;
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  aosf33_mulf33
    (&f33,
     (struct aosf33[]) {{f44a.c0, f44a.c1, f44a.c2}},
     (struct aosf33[]) {{
      vf4_set(0.5f, 0.f, 0.f, 0.f),
      vf4_set(0.f, 0.789f, 0.f, 0.f),
      vf4_set(0.f, 0.f, -2.78f, 0.f)}});
  f44a.c0 = f33.c0;
  f44a.c1 = f33.c1;
  f44a.c2 = f33.c2;
  aosf44_store(tmp, &f44a);
  CHECK_TRANSFORM(inst1,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);

  CHECK(rdr_transform_model_instances
    ((struct rdr_model_instance*[]){inst, inst1}, 2, false,
     (struct aosf44[]){{
      vf4_set(45.f, 4.f, 7.8f, 0.f),
      vf4_set(5.1f, 2.f, -6.f, 0.f),
      vf4_set(0.5f, -50.f, 899.f, 0.f),
      vf4_set(4.5f, 0.01f, -989.f, 1.f)}}), OK);
  aosf44_mulf44
    (&f44,
     (struct aosf44[]){{
      vf4_set(45.f, 4.f, 7.8f, 0.f),
      vf4_set(5.1f, 2.f, -6.f, 0.f),
      vf4_set(0.5f, -50.f, 899.f, 0.f),
      vf4_set(4.5f, 0.01f, -989.f, 1.f)}},
     &f44);
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  aosf44_mulf44
    (&f44a,
     (struct aosf44[]){{
      vf4_set(45.f, 4.f, 7.8f, 0.f),
      vf4_set(5.1f, 2.f, -6.f, 0.f),
      vf4_set(0.5f, -50.f, 899.f, 0.f),
      vf4_set(4.5f, 0.01f, -989.f, 1.f)}},
     &f44a);
  aosf44_store(tmp, &f44a);
  CHECK_TRANSFORM(inst1,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(rdr_transform_model_instances
    ((struct rdr_model_instance*[]){inst, inst1}, 2, true,
     (struct aosf44[]){{
      vf4_set(45.f, 4.f, 7.8f, 0.f),
      vf4_set(5.1f, 2.f, -6.f, 0.f),
      vf4_set(0.5f, -50.f, 899.f, 0.f),
      vf4_set(4.5f, 0.01f, -989.f, 1.f)}}), OK);
  aosf44_mulf44
    (&f44, &f44,
     (struct aosf44[]){{
      vf4_set(45.f, 4.f, 7.8f, 0.f),
      vf4_set(5.1f, 2.f, -6.f, 0.f),
      vf4_set(0.5f, -50.f, 899.f, 0.f),
      vf4_set(4.5f, 0.01f, -989.f, 1.f)}});
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  aosf44_mulf44
    (&f44a, &f44a,
     (struct aosf44[]){{
      vf4_set(45.f, 4.f, 7.8f, 0.f),
      vf4_set(5.1f, 2.f, -6.f, 0.f),
      vf4_set(0.5f, -50.f, 899.f, 0.f),
      vf4_set(4.5f, 0.01f, -989.f, 1.f)}});
  aosf44_store(tmp, &f44a);
  CHECK_TRANSFORM(inst1,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);

  CHECK(rdr_move_model_instances
    ((struct rdr_model_instance*[]){inst, inst1}, 2,
     (float[]){42.4f, -0.002f, 5.7f}), OK);
  f44.c3 = vf4_set(42.4f, -0.002f, 5.7f, 1.f);
  aosf44_store(tmp, &f44);
  CHECK_TRANSFORM(inst,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);
  f44a.c3 = vf4_set(42.4f, -0.002f, 5.7f, 1.f);
  aosf44_store(tmp, &f44a);
  CHECK_TRANSFORM(inst1,
     tmp[0], tmp[1], tmp[2], tmp[3],
     tmp[4], tmp[5], tmp[6], tmp[7],
     tmp[8], tmp[9], tmp[10], tmp[11],
     tmp[12], tmp[13], tmp[14], tmp[15]);

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

  test_bounding(sys);

  CHECK(rdr_system_ref_put(sys), OK);
  CHECK(wm_window_ref_put(window), WM_NO_ERROR);
  CHECK(wm_device_ref_put(device), WM_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;
}

