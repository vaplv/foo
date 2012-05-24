#include "app/core/app.h"
#include "app/core/app_model.h"
#include "app/core/app_model_instance.h"
#include "app/core/app_world.h"
#include "maths/simd/aosf44.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "utest/app/core/cube_obj.h"
#include "utest/utest.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH "/tmp/cube.obj"

#define OK APP_NO_ERROR
#define BAD_ARG APP_INVALID_ARGUMENT

#define CHECK_EPS(a, b) \
  CHECK(fabsf((a) - (b)) < 1.e-6f, true)

#define EPS 1.e-8f
#define CHECK_TRANSFORM(inst, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
  do { \
    ALIGN(16) float m__[16]; \
    const struct aosf44* f44__ = NULL; \
    CHECK(app_get_raw_model_instance_transform(inst, &f44__), OK); \
    aosf44_store(m__, f44__); \
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

static void
cbk(struct app_model* model, void* data)
{
  struct app_model** m = data;
  (*m) = model;
}

static void
test_app_model_instance_transform(struct app* app)
{
  struct aosf44 f44;
  struct aosf44 f44a;
  struct aosf33 f33;
  struct aosf33 f33a;
  ALIGN(16) float tmp[16];
  const struct aosf44* f44_ptr = NULL;
  struct app_model* model = NULL;
  struct app_model_instance* inst = NULL;
  struct app_model_instance* inst1 = NULL;

  CHECK(app_create_model(app, PATH, NULL, &model), OK);
  CHECK(app_instantiate_model(app, model, NULL, &inst), OK);

  CHECK(app_get_raw_model_instance_transform(NULL, NULL), BAD_ARG);
  CHECK(app_get_raw_model_instance_transform(inst, NULL), BAD_ARG);
  CHECK(app_get_raw_model_instance_transform(NULL, &f44_ptr), BAD_ARG);
  CHECK_TRANSFORM(inst,
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f);

  CHECK(app_translate_model_instances(NULL, 1, false, NULL), BAD_ARG);
  CHECK(app_translate_model_instances(&inst, 1, false, NULL), BAD_ARG);
  CHECK(app_translate_model_instances
    (NULL, 1, false, (float[]){1.f, 5.f, 3.f}), BAD_ARG);
  CHECK(app_translate_model_instances
    (&inst, 1, false, (float[]){1.f, 5.f, 3.f}), OK);
  CHECK_TRANSFORM(inst,
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    1.f, 5.f, 3.f, 1.f);
  CHECK(app_translate_model_instances
    (&inst, 1, false, (float[]){-4.f, 2.1f, 1.f}), OK);
  CHECK_TRANSFORM(inst,
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    -3.f, 7.1f, 4.f, 1.f);
  CHECK(app_translate_model_instances
    (&inst, 0, false, (float[]){-4.f, 2.1f, 1.f}), OK);
  CHECK_TRANSFORM(inst,
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    -3.f, 7.1f, 4.f, 1.f);
  CHECK(app_translate_model_instances
    (NULL, 0, false, (float[]){-4.f, 2.1f, 1.f}), OK);
  CHECK_TRANSFORM(inst,
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    -3.f, 7.1f, 4.f, 1.f);

  CHECK(app_move_model_instances(NULL, 0, NULL), BAD_ARG);
  CHECK(app_move_model_instances(&inst, 0, NULL), BAD_ARG);
  CHECK(app_move_model_instances(NULL, 1, NULL), BAD_ARG);
  CHECK(app_move_model_instances(&inst, 1, NULL), BAD_ARG);
  CHECK(app_move_model_instances(NULL, 0, (float[]){0.f, 0.f, 0.f}), OK);
  CHECK_TRANSFORM(inst,
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    -3.f, 7.1f, 4.f, 1.f);
  CHECK(app_move_model_instances(&inst, 0, (float[]){0.f, 0.f, 0.f}), OK);
  CHECK_TRANSFORM(inst,
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    -3.f, 7.1f, 4.f, 1.f);
  CHECK(app_move_model_instances(NULL, 1, (float[]){0.f, 0.f, 0.f}), BAD_ARG);
  CHECK(app_move_model_instances(&inst, 1, (float[]){0.f, 0.f, 0.f}), OK);
  CHECK_TRANSFORM(inst,
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f);
  CHECK(app_move_model_instances(&inst, 1, (float[]){5.f, 3.1f, -1.8f}), OK);
  CHECK_TRANSFORM(inst,
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    5.f, 3.1f, -1.8f, 1.f);
  CHECK(app_move_model_instances(&inst, 1, (float[]){0.f, 0.f, 0.f}), OK);

  CHECK(app_rotate_model_instances(NULL, 0, false, NULL), BAD_ARG);
  CHECK(app_rotate_model_instances(&inst, 0, false, NULL), BAD_ARG);
  CHECK(app_rotate_model_instances
    (NULL, 0, false, (float[]){0.785f, 0.f, 0.f}), OK);
  CHECK_TRANSFORM(inst,
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f);
  CHECK(app_rotate_model_instances
    (&inst, 0, false, (float[]){0.785f, 0.f, 0.f}), OK);
  CHECK_TRANSFORM(inst,
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f);
  CHECK(app_rotate_model_instances
    (&inst, 1, false, (float[]){0.785f, 0.f, 0.f}), OK);
  aosf33_rotation(&f33, 0.785f, 0.f, 0.f);
  f44.c0 = f33.c0;
  f44.c1 = f33.c1;
  f44.c2 = f33.c2;
  f44.c3 = vf4_set(0.f, 0.f, 0.f, 1.f);
  CHECK(app_get_raw_model_instance_transform(inst, &f44_ptr), OK);
  aosf44_store(tmp, f44_ptr);
  CHECK_TRANSFORM(inst,
    tmp[0], tmp[1], tmp[2], tmp[3],
    tmp[4], tmp[5], tmp[6], tmp[7],
    tmp[8], tmp[9], tmp[10], tmp[11],
    tmp[12], tmp[13], tmp[14], tmp[15]);

  CHECK(app_translate_model_instances
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
  CHECK(app_translate_model_instances
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
  CHECK(app_rotate_model_instances
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
  CHECK(app_rotate_model_instances
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

  CHECK(app_scale_model_instances(NULL, 0, false, NULL), BAD_ARG);
  CHECK(app_scale_model_instances(&inst, 0, false, NULL), BAD_ARG);
  CHECK(app_scale_model_instances
    (NULL, 0, true, (float[]){5.f, 4.f, 2.1f}), OK);
  CHECK_TRANSFORM(inst,
    tmp[0], tmp[1], tmp[2], tmp[3],
    tmp[4], tmp[5], tmp[6], tmp[7],
    tmp[8], tmp[9], tmp[10], tmp[11],
    tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(app_scale_model_instances
    (&inst, 0, true, (float[]){5.f, 4.f, 2.1f}), OK);
  CHECK_TRANSFORM(inst,
    tmp[0], tmp[1], tmp[2], tmp[3],
    tmp[4], tmp[5], tmp[6], tmp[7],
    tmp[8], tmp[9], tmp[10], tmp[11],
    tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(app_scale_model_instances
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
  CHECK(app_scale_model_instances
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

  CHECK(app_transform_model_instances(NULL, 0, true, NULL), BAD_ARG);
  CHECK(app_transform_model_instances(&inst, 0, true, NULL), BAD_ARG);
  CHECK(app_transform_model_instances(NULL, 0, true, &f44), OK);
  CHECK_TRANSFORM(inst,
    tmp[0], tmp[1], tmp[2], tmp[3],
    tmp[4], tmp[5], tmp[6], tmp[7],
    tmp[8], tmp[9], tmp[10], tmp[11],
    tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(app_transform_model_instances(&inst, 0, true, &f44), OK);
  CHECK_TRANSFORM(inst,
    tmp[0], tmp[1], tmp[2], tmp[3],
    tmp[4], tmp[5], tmp[6], tmp[7],
    tmp[8], tmp[9], tmp[10], tmp[11],
    tmp[12], tmp[13], tmp[14], tmp[15]);
  CHECK(app_transform_model_instances
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
  CHECK(app_transform_model_instances
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

  /* Test the transformation of a list of instances. */
  CHECK(app_instantiate_model(app, model, NULL, &inst1), OK);
  CHECK(app_get_raw_model_instance_transform(inst1, &f44_ptr), OK);
  f44a = *f44_ptr;

  CHECK(app_translate_model_instances
    ((struct app_model_instance*[]){inst, inst1}, 2,
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
  CHECK(app_translate_model_instances
    ((struct app_model_instance*[]){inst, inst1}, 2,
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

  CHECK(app_rotate_model_instances
    ((struct app_model_instance*[]){inst, inst1}, 2,
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
  CHECK(app_rotate_model_instances
    ((struct app_model_instance*[]){inst, inst1}, 2,
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

  CHECK(app_scale_model_instances
    ((struct app_model_instance*[]){inst, inst1}, 2,
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
  CHECK(app_scale_model_instances
    ((struct app_model_instance*[]){inst, inst1}, 2,
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

  CHECK(app_transform_model_instances
    ((struct app_model_instance*[]){inst, inst1}, 2, false,
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
  CHECK(app_transform_model_instances
    ((struct app_model_instance*[]){inst, inst1}, 2, true,
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

  CHECK(app_move_model_instances
    ((struct app_model_instance*[]){inst, inst1}, 2,
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

  CHECK(app_model_instance_ref_put(inst), OK);
  CHECK(app_model_instance_ref_put(inst1), OK);
  CHECK(app_model_ref_put(model), OK);
}

static void
test_app_model_instance_bound(struct app* app)
{
  const float data[] = { /* Data copied from the cube_obj.h ressource. */
    -0.664393f, 2.250566f, 0.199463f,
    0.335607f, 2.250566f, 0.199463f,
    -0.664393f, 3.250566f, 0.199463f,
    0.335607f, 3.250566f, 0.199463f,
    -0.664393f, 3.250566f, -0.800537f,
    0.335607f, 3.250566f, -0.800537f,
    -0.664393f, 2.250566f, -0.800537f,
    0.335607f, 2.250566f, -0.800537f
  };
  struct aosf44 f44;
  struct aosf33 f33;
  ALIGN(16) float array[4];
  vf4_t vec0;
  vf4_t vec1;
  vf4_t vec2;
  vf4_t vec3;
  float pos[3];
  float extx[3];
  float exty[3];
  float extz[3];
  float min_bound[3];
  float max_bound[3];
  struct app_model* model = NULL;
  struct app_model_instance* inst = NULL;
  size_t i = 0;

  CHECK(app_create_model(app, PATH, NULL, &model), OK);
  CHECK(app_instantiate_model(app, model, NULL, &inst), OK);

  CHECK(app_get_model_instance_aabb(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance_aabb(inst, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance_aabb(NULL, min_bound, NULL), BAD_ARG);
  CHECK(app_get_model_instance_aabb(inst, min_bound, NULL), BAD_ARG);
  CHECK(app_get_model_instance_aabb(NULL, NULL, max_bound), BAD_ARG);
  CHECK(app_get_model_instance_aabb(inst, NULL, max_bound), BAD_ARG);
  CHECK(app_get_model_instance_aabb(NULL, min_bound, max_bound), BAD_ARG);
  CHECK(app_get_model_instance_aabb(inst, min_bound, max_bound), OK);
  CHECK_EPS(min_bound[0], -0.664393f);
  CHECK_EPS(min_bound[1], 2.250566f);
  CHECK_EPS(min_bound[2], -0.800537f);
  CHECK_EPS(max_bound[0], 0.335607f);
  CHECK_EPS(max_bound[1], 3.250566f);
  CHECK_EPS(max_bound[2], 0.199463f);

  CHECK(app_get_model_instance_obb(NULL, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, pos, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, pos, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, NULL, extx, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, NULL, extx, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, pos, extx, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, pos, extx, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, NULL, NULL, exty, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, NULL, NULL, exty, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, pos, NULL, exty, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, pos, NULL, exty, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, NULL, extx, exty, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, NULL, extx, exty, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, pos, extx, exty, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, pos, extx, exty, NULL), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, NULL, NULL, NULL, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, NULL, NULL, NULL, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, pos, NULL, NULL, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, pos, NULL, NULL, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, NULL, extx, NULL, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, NULL, extx, NULL, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, pos, extx, NULL, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, pos, extx, NULL, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, NULL, NULL, exty, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, NULL, NULL, exty, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, pos, NULL, exty, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, pos, NULL, exty, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, NULL, extx, exty, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, NULL, extx, exty, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(NULL, pos, extx, exty, extz), BAD_ARG);
  CHECK(app_get_model_instance_obb(inst, pos, extx, exty, extz), OK);
  CHECK_EPS(pos[0], (min_bound[0] + max_bound[0]) * 0.5f);
  CHECK_EPS(pos[1], (min_bound[1] + max_bound[1]) * 0.5f);
  CHECK_EPS(pos[2], (min_bound[2] + max_bound[2]) * 0.5f);
  CHECK_EPS(extx[0], (max_bound[0] - min_bound[0]) * 0.5f);
  CHECK_EPS(extx[1], 0.f);
  CHECK_EPS(extx[2], 0.f);
  CHECK_EPS(exty[0], 0.f);
  CHECK_EPS(exty[1], (max_bound[1] - min_bound[1]) * 0.5f);
  CHECK_EPS(exty[2], 0.f);
  CHECK_EPS(extz[0], 0.f);
  CHECK_EPS(extz[1], 0.f);
  CHECK_EPS(extz[2], (max_bound[2] - min_bound[2]) * 0.5f);

  aosf33_rotation(&f33, 0.785f, 0.11f, 0.87f);
  aosf44_set(&f44, f33.c0, f33.c1, f33.c2, vf4_set(15.878f, -5.68f, 0.25f, 1.f));
  CHECK(app_transform_model_instances(&inst, 1, true, &f44), OK);
  CHECK(app_get_model_instance_aabb(inst, min_bound, max_bound), OK);

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
  CHECK(app_get_model_instance_obb(inst, pos, extx, exty, extz), OK);
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

  CHECK(app_model_ref_put(model), OK);
  CHECK(app_model_instance_ref_put(inst), OK);
}

int
main(int argc, char** argv)
{
  #define MODEL_COUNT 5
  struct app_args args = { NULL, NULL, NULL, NULL };
  struct app* app = NULL;
  struct app_model* model = NULL;
  struct app_model* model2 = NULL;
  struct app_model* model3 = NULL;
  struct app_model* model_list[MODEL_COUNT];
  struct app_model_it model_it;
  struct app_model_instance* instance = NULL;
  struct app_model_instance* instance1 = NULL;
  struct app_model_instance_it instance_it;
  struct app_world* world0 = NULL;
  struct app_world* world1 = NULL;
  const char** lst = NULL;
  const char* cstr = NULL;
  FILE* fp = NULL;
  size_t i = 0;
  size_t len = 0;
  bool b = false;

  if(argc != 2) {
    printf("usage: %s RB_DRIVER\n", argv[0]);
    return -1;
  }
  args.render_driver = argv[1];

  /* Create the temporary obj file. */
  fp = fopen(PATH, "w");
  NCHECK(fp, NULL);
  i = fwrite(cube_obj, sizeof(char), strlen(cube_obj), fp);
  CHECK(strlen(cube_obj) * sizeof(char), i);
  CHECK(fclose(fp), 0);

  CHECK(app_init(&args, &app), OK);

  CHECK(app_attach_callback
    (NULL, APP_SIGNAL_CREATE_MODEL, NULL, NULL), BAD_ARG);
  CHECK(app_attach_callback
    (app, APP_SIGNAL_CREATE_MODEL, NULL, NULL), BAD_ARG);
  CHECK(app_attach_callback
    (NULL, APP_SIGNAL_CREATE_MODEL, APP_CALLBACK(cbk), NULL), BAD_ARG);
  CHECK(app_attach_callback
    (app, APP_SIGNAL_CREATE_MODEL, APP_CALLBACK(cbk), NULL), OK);

  CHECK(app_detach_callback
    (NULL, APP_SIGNAL_DESTROY_MODEL, NULL, NULL), BAD_ARG);
  CHECK(app_detach_callback
    (app, APP_SIGNAL_DESTROY_MODEL, NULL, NULL), BAD_ARG);
  CHECK(app_detach_callback
    (NULL, APP_SIGNAL_CREATE_MODEL, NULL, NULL), BAD_ARG);
  CHECK(app_detach_callback
    (app, APP_SIGNAL_CREATE_MODEL, NULL, NULL), BAD_ARG);
  CHECK(app_detach_callback
    (NULL, APP_SIGNAL_DESTROY_MODEL, APP_CALLBACK(cbk), NULL), BAD_ARG);
  CHECK(app_detach_callback
    (app, APP_SIGNAL_DESTROY_MODEL, APP_CALLBACK(cbk), NULL), BAD_ARG);
  CHECK(app_detach_callback
    (NULL, APP_SIGNAL_CREATE_MODEL, APP_CALLBACK(cbk), NULL), BAD_ARG);
  CHECK(app_detach_callback
    (app, APP_SIGNAL_CREATE_MODEL, APP_CALLBACK(cbk), NULL), OK);

  CHECK(app_attach_callback
    (app, APP_SIGNAL_CREATE_MODEL, APP_CALLBACK(cbk), &model2), OK);
  CHECK(app_attach_callback
    (app, APP_SIGNAL_DESTROY_MODEL, APP_CALLBACK(cbk), &model3), OK);

  model2 = NULL;
  CHECK(app_create_model(NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_create_model(app, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_create_model(NULL, NULL, NULL, &model), BAD_ARG);
  CHECK(app_create_model(app, NULL, NULL, &model), OK);
  CHECK(model, model2);

  model3 = NULL;
  CHECK(app_model_ref_put(NULL), BAD_ARG);
  CHECK(app_model_ref_put(model), OK);
  CHECK(model, model3);

  model2 = NULL;
  CHECK(app_create_model(app, PATH, NULL, &model), OK);
  CHECK(model, model2);

  model2 = NULL;
  CHECK(app_clear_model(NULL), BAD_ARG);
  CHECK(app_clear_model(model), OK);
  CHECK(model2, NULL);

  CHECK(app_load_model(NULL, NULL), BAD_ARG);
  CHECK(app_load_model(PATH, NULL), BAD_ARG);
  CHECK(app_load_model(NULL, model), BAD_ARG);
  CHECK(app_load_model(PATH, model), OK);

  CHECK(app_model_path(NULL, NULL), BAD_ARG);
  CHECK(app_model_path(model, NULL), BAD_ARG);
  CHECK(app_model_path(NULL, &cstr), BAD_ARG);
  CHECK(app_model_path(model, &cstr), OK);
  CHECK(strcmp(cstr, PATH), 0);

  CHECK(app_set_model_name(NULL, NULL), BAD_ARG);
  CHECK(app_set_model_name(model, NULL), BAD_ARG);
  CHECK(app_set_model_name(NULL, "Mdl0"), BAD_ARG);
  CHECK(app_set_model_name(model, "Mdl0"), OK);
  CHECK(app_set_model_name(model, "Mdl0"), OK); /* Set the same name. */
  CHECK(model2, NULL);

  CHECK(app_model_name(NULL, NULL), BAD_ARG);
  CHECK(app_model_name(model, NULL), BAD_ARG);
  CHECK(app_model_name(NULL, &cstr), BAD_ARG);
  CHECK(app_model_name(model, &cstr), OK);
  CHECK(strcmp(cstr, "Mdl0"), 0);

  CHECK(app_get_model(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_get_model(app, NULL, NULL), BAD_ARG);
  CHECK(app_get_model(NULL, "Mdl0", NULL), BAD_ARG);
  CHECK(app_get_model(app, "Mdl0", NULL), BAD_ARG);
  CHECK(app_get_model(NULL, NULL, &model), BAD_ARG);
  CHECK(app_get_model(app, NULL, &model), BAD_ARG);
  CHECK(app_get_model(NULL, "Mdl0", &model), BAD_ARG);
  CHECK(app_get_model(app, "Mdl0", &model), OK);
  CHECK(app_model_name(model, &cstr), OK);
  CHECK(strcmp(cstr, "Mdl0"), 0);

  /* This model may be freed when the application will be shut down. */
  for(i = 0; i < MODEL_COUNT; ++i) {
    char name[8];
    STATIC_ASSERT(MODEL_COUNT < 9999, Unexpected_MODEL_COUNT);
    snprintf(name, 7, "mdl%zu", i + 1);
    name[7] = '\0';
    CHECK(app_create_model(app, PATH, name, &model_list[i]), OK);
    CHECK(app_model_name(model_list[i], &cstr), OK);
    CHECK(strcmp(cstr, name), 0);
  }

  CHECK(app_model_name_completion(NULL, NULL, 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_name_completion(app, NULL, 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_name_completion(NULL, "M", 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_name_completion(app, "M", 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_name_completion(NULL, NULL, 0, &len, NULL), BAD_ARG);
  CHECK(app_model_name_completion(app, NULL, 0, &len, NULL), BAD_ARG);
  CHECK(app_model_name_completion(NULL, "M", 0, &len, NULL), BAD_ARG);
  CHECK(app_model_name_completion(app, "M", 0, &len, NULL), BAD_ARG);
  CHECK(app_model_name_completion(NULL, NULL, 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_name_completion(app, NULL, 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_name_completion(NULL, "M", 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_name_completion(app, "M", 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_name_completion(NULL, NULL, 0, &len, &lst), BAD_ARG);
  CHECK(app_model_name_completion(app, NULL, 1, &len, &lst), BAD_ARG);
  CHECK(app_model_name_completion(app, NULL, 0, &len, &lst), OK);
  CHECK(len, MODEL_COUNT + 1);
  NCHECK(lst, NULL);
  for(i = 1; i < MODEL_COUNT + 1; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
  }
  CHECK(app_model_name_completion(NULL, "M", 0, &len, &lst), BAD_ARG);
  CHECK(app_model_name_completion(app, "M", 0, &len, &lst), OK);
  CHECK(len, MODEL_COUNT + 1);
  NCHECK(lst, NULL);
  for(i = 1; i < MODEL_COUNT + 1; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
  }
  CHECK(app_model_name_completion(app, "Mxx", 1, &len, &lst), OK);
  CHECK(len, 1);
  NCHECK(lst, NULL);
  CHECK(strcmp(lst[0], "Mdl0"), 0);
  CHECK(app_model_name_completion(app, "Mxx", 2, &len, &lst), OK);
  CHECK(len, 0);
  CHECK(lst, NULL);
  CHECK(app_model_name_completion(app, "mdl", 3, &len, &lst), OK);
  CHECK(len, MODEL_COUNT);
  NCHECK(lst, NULL);
  CHECK(strncmp(lst[0], "mdl", 3), 0);
  for(i = 1; i < MODEL_COUNT; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
    CHECK(strncmp(lst[i], "mdl", 3), 0);
  }

  CHECK(app_instantiate_model(NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_instantiate_model(app, NULL, NULL, NULL), BAD_ARG);
  CHECK(app_instantiate_model(NULL, model, NULL, NULL), BAD_ARG);
  CHECK(app_instantiate_model(app, model, NULL, NULL), OK);
  CHECK(app_instantiate_model(NULL, NULL, "inst0", NULL), BAD_ARG);
  CHECK(app_instantiate_model(app, NULL, "inst0", NULL), BAD_ARG);
  CHECK(app_instantiate_model(NULL, model, "inst0", NULL), BAD_ARG);
  CHECK(app_instantiate_model(app, model, "inst0", NULL), OK);
  CHECK(app_instantiate_model(NULL, NULL, NULL, &instance), BAD_ARG);
  CHECK(app_instantiate_model(app, NULL, NULL, &instance), BAD_ARG);
  CHECK(app_instantiate_model(NULL, model, NULL, &instance), BAD_ARG);
  CHECK(app_instantiate_model(app, model, NULL, &instance), OK);
  CHECK(app_instantiate_model(NULL, NULL, "inst1", &instance1), BAD_ARG);
  CHECK(app_instantiate_model(app, NULL, "inst1", &instance1), BAD_ARG);
  CHECK(app_instantiate_model(NULL, model, "inst1", &instance1), BAD_ARG);
  CHECK(app_instantiate_model(app, model, "inst1", &instance1), OK);

  CHECK(app_set_model_instance_name(NULL, NULL), BAD_ARG);
  CHECK(app_set_model_instance_name(instance, NULL), BAD_ARG);
  CHECK(app_set_model_instance_name(NULL, "my_inst0"), BAD_ARG);
  CHECK(app_set_model_instance_name(instance, "my_inst0"), OK);

  CHECK(app_model_instance_name(NULL, NULL), BAD_ARG);
  CHECK(app_model_instance_name(instance, NULL), BAD_ARG);
  CHECK(app_model_instance_name(NULL, &cstr), BAD_ARG);
  CHECK(app_model_instance_name(instance, &cstr), OK);
  CHECK(strcmp(cstr, "my_inst0"), 0);

  CHECK(app_get_model_instance(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance(app, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance(NULL, "my_inst0", NULL), BAD_ARG);
  CHECK(app_get_model_instance(app, "my_inst0", NULL), BAD_ARG);
  CHECK(app_get_model_instance(NULL, NULL, &instance), BAD_ARG);
  CHECK(app_get_model_instance(app, NULL, &instance), BAD_ARG);
  CHECK(app_get_model_instance(NULL, "my_inst0", &instance), BAD_ARG);
  CHECK(app_get_model_instance(app, "my_inst0", &instance), OK);
  CHECK(app_model_instance_name(instance, &cstr), OK);
  CHECK(strcmp(cstr, "my_inst0"), 0);

  CHECK(app_instantiate_model(app, model, "my_inst", NULL), OK);
  CHECK(app_instantiate_model(app, model, "inst1", NULL), OK);
  CHECK(app_instantiate_model(app, model, "myinst", NULL), OK);

  CHECK(app_model_instance_name_completion(NULL, NULL, 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, NULL, 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(NULL, "m", 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, "m", 0, NULL, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(NULL, NULL, 0, &len, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, NULL, 0, &len, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(NULL, "m", 0, &len, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, "m", 0, &len, NULL), BAD_ARG);
  CHECK(app_model_instance_name_completion(NULL, NULL, 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, NULL, 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(NULL, "m", 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, "m", 0, NULL, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(NULL, NULL, 0, &len, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, NULL, 1, &len, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, NULL, 0, &len, &lst), OK);
  CHECK(len, 7);
  for(i = 1; i < len; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
  }
  CHECK(app_model_instance_name_completion(NULL, "m", 0, &len, &lst), BAD_ARG);
  CHECK(app_model_instance_name_completion(app, "m", 0, &len, &lst), OK);
  CHECK(len, 7);
  for(i = 1; i < len; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
  }
  CHECK(app_model_instance_name_completion(app, "my", 2, &len, &lst), OK);
  CHECK(len < 7, true);
  CHECK(strncmp(lst[0], "my", 2), 0);
  for(i = 1; i < len; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
    CHECK(strncmp(lst[i], "my", 2), 0);
  }
  i = len;
  CHECK(app_model_instance_name_completion(app, "my_", 3, &len, &lst), OK);
  CHECK(len < i, true);
  CHECK(strncmp(lst[0], "my_", 3), 0);
  for(i = 1; i < len; ++i) {
    CHECK(strcmp(lst[i-1], lst[i]) <= 0, true);
    CHECK(strncmp(lst[i], "my_", 3), 0);
  }

  CHECK(app_model_instance_ref_get(NULL), BAD_ARG);
  CHECK(app_model_instance_ref_get(instance), OK);
  CHECK(app_model_instance_ref_put(NULL), BAD_ARG);
  CHECK(app_get_main_world(app, &world0), OK);
  CHECK(app_world_add_model_instances(world0, 1, &instance), OK);

  CHECK(app_model_instance_world(NULL, NULL), BAD_ARG);
  CHECK(app_model_instance_world(instance, NULL), BAD_ARG);
  CHECK(app_model_instance_world(NULL, &world1), BAD_ARG);
  CHECK(app_model_instance_world(instance, &world1), OK);
  CHECK(world1, world0);

  /* Check that the instance is registered. */
  CHECK(app_get_model_instance_list_begin(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance_list_begin(app, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance_list_begin(NULL, &instance_it, NULL), BAD_ARG);
  CHECK(app_get_model_instance_list_begin(app, &instance_it, NULL), BAD_ARG);
  CHECK(app_get_model_instance_list_begin(NULL, NULL, &b), BAD_ARG);
  CHECK(app_get_model_instance_list_begin(app, NULL, &b), BAD_ARG);
  CHECK(app_get_model_instance_list_begin(NULL, &instance_it, &b), BAD_ARG);

  CHECK(app_model_instance_it_next(NULL, NULL), BAD_ARG);
  CHECK(app_model_instance_it_next(&instance_it, NULL), BAD_ARG);
  CHECK(app_model_instance_it_next(NULL, &b), BAD_ARG);

  CHECK(app_get_model_instance_list_begin(app, &instance_it, &b), OK);
  i = 0;
  while(b == false) {
    if(instance_it.instance == instance)
      break;
    CHECK(app_model_instance_it_next(&instance_it, &b), OK);
    ++i;
  }
  CHECK(app_get_model_instance_list_length(NULL, NULL), BAD_ARG);
  CHECK(app_get_model_instance_list_length(app, NULL), BAD_ARG);
  CHECK(app_get_model_instance_list_length(NULL, &len), BAD_ARG);
  CHECK(app_get_model_instance_list_length(app, &len), OK);
  CHECK(len, 7);
  CHECK(i < len, true);
  CHECK(app_remove_model_instance(NULL), BAD_ARG);
  CHECK(app_remove_model_instance(instance), OK);
  /* Check that the instance is *NOT* registered. */
  CHECK(app_get_model_instance_list_begin(app, &instance_it, &b), OK);
  i = 0;
  while(b == false) {
    if(instance_it.instance == instance)
      break;
    CHECK(app_model_instance_it_next(&instance_it, &b), OK);
    ++i;
  }
  CHECK(app_get_model_instance_list_length(app, &len), OK);
  CHECK(len, 6);
  CHECK(i < len, false);
  /* Even though it is not registered the instance is still valid since we get
   * a reference onto it. */
  CHECK(app_model_instance_name(instance, &cstr), OK);
  CHECK(strcmp(cstr, "my_inst0"), 0);
  CHECK(app_model_instance_world(instance, &world1), OK);
  NCHECK(world0, world1);
  CHECK(world1, NULL);
  CHECK(app_model_instance_ref_put(instance), OK);

  /* Instantiate a new model. */
  CHECK(app_create_model(app, PATH, NULL, &model2), OK);
  CHECK(app_is_model_instantiated(NULL, NULL), BAD_ARG);
  CHECK(app_is_model_instantiated(model2, NULL), BAD_ARG);
  CHECK(app_is_model_instantiated(NULL, &b), BAD_ARG);
  CHECK(app_is_model_instantiated(model2, &b), OK);
  CHECK(b, false);
  CHECK(app_instantiate_model(app, model2, NULL, &instance), OK);
  CHECK(app_is_model_instantiated(model2, &b), OK);
  CHECK(b, true);
  CHECK(app_remove_model_instance(instance), OK);
  CHECK(app_is_model_instantiated(model2, &b), OK);
  CHECK(b, false);
  CHECK(app_instantiate_model(app, model2, NULL, NULL), OK);
  CHECK(app_instantiate_model(app, model2, NULL, NULL), OK);

  /* Check that model is registered. */
  CHECK(app_get_model_list_begin(NULL, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_list_begin(app, NULL, NULL), BAD_ARG);
  CHECK(app_get_model_list_begin(NULL, &model_it, NULL), BAD_ARG);
  CHECK(app_get_model_list_begin(app, &model_it, NULL), BAD_ARG);
  CHECK(app_get_model_list_begin(NULL, NULL, &b), BAD_ARG);
  CHECK(app_get_model_list_begin(app, NULL, &b), BAD_ARG);
  CHECK(app_get_model_list_begin(NULL, &model_it, &b), BAD_ARG);

  CHECK(app_model_it_next(NULL, NULL), BAD_ARG);
  CHECK(app_model_it_next(&model_it, NULL), BAD_ARG);
  CHECK(app_model_it_next(NULL, &b), BAD_ARG);

  CHECK(app_get_model_list_begin(app, &model_it, &b), OK);
  i = 0;
  while(b == false) {
    if(model_it.model == model)
      break;
    CHECK(app_model_it_next(&model_it, &b), OK);
    ++i;
  }
  CHECK(app_get_model_list_length(NULL, NULL), BAD_ARG);
  CHECK(app_get_model_list_length(app, NULL), BAD_ARG);
  CHECK(app_get_model_list_length(NULL, &len), BAD_ARG);
  CHECK(app_get_model_list_length(app, &len), OK);
  CHECK(i < len, true);
  /* Get a ref onto the model and then remove the model. */
  CHECK(app_model_ref_get(NULL), BAD_ARG);
  CHECK(app_model_ref_get(model), OK);
  CHECK(app_remove_model(NULL), BAD_ARG);
  CHECK(app_remove_model(model), OK);

  /* The model must be unregistered. */
  CHECK(app_get_model_list_begin(app, &model_it, &b), OK);
  i = 0;
  while(b == false) {
    if(model_it.model == model)
      break;
    CHECK(app_model_it_next(&model_it, &b), OK);
    ++i;
  }
  CHECK(app_get_model_list_length(app, &len), OK);
  CHECK(i < len, false);
  /* Check that even though the model is removed it is still valid since we
   * previously owned it. */
  CHECK(app_model_name(model, &cstr), OK);
  CHECK(strcmp(cstr, "Mdl0"), 0);
  /* Check that all the instances of the model are alse removed from the
   * application. */
  CHECK(app_get_model_instance_list_length(app, &len), OK);
  CHECK(len, 2);

  CHECK(app_model_ref_put(model), OK);

  CHECK(app_cleanup(app), OK);

  test_app_model_instance_transform(app);
  test_app_model_instance_bound(app);
  CHECK(app_ref_put(app), OK);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;
}

