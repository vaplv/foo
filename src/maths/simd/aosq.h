/* The quaternion is encoded into a vf4_t as { i, j, k, a }. */
#ifndef AOSQ_H
#define AOSQ_H

#include "maths/simd/simd.h"

struct aosf33;

/* Set operations. */
static FINLINE vf4_t
aosq_identity(void)
{
  return vf4_set(0.f, 0.f, 0.f, 1.f);
}

static FINLINE vf4_t
aosq_set_axis_angle(vf4_t xyz_, vf4_t aaaa)
{
  const vf4_t half_angle = vf4_mul(aaaa, vf4_set1(0.5f));
  vf4_t s, c;
  vf4_sincos(half_angle, &s, &c);

  const vf4_t axis1 = vf4_xyzd(xyz_, vf4_set1(1.f));
  const vf4_t sssc = vf4_xyzd(s, c);

  /* { x*sin(a/2), y*sin(a/2), z*sin(a/2), cos(a/2) } */
  return vf4_mul(axis1, sssc);
}

/* Comparison operations. */
static FINLINE vf4_t
aosq_eq(vf4_t q0, vf4_t q1)
{
  const vf4_t r0 = vf4_eq(q0, q1);
  const vf4_t r1 = vf4_and(vf4_xxyy(r0), vf4_zzww(r0));
  return vf4_and(vf4_xxyy(r1), vf4_zzww(r1));
}

static FINLINE vf4_t
aosq_eq_eps(vf4_t q0, vf4_t q1, vf4_t eps)
{
  const vf4_t r0 = vf4_eq_eps(q0, q1, eps);
  const vf4_t r1 = vf4_and(vf4_xxyy(r0), vf4_zzww(r0));
  return vf4_and(vf4_xxyy(r1), vf4_zzww(r1));
}

/* Arithmetic operations. */
static FINLINE vf4_t
aosq_mul(vf4_t q0, vf4_t q1)
{
  const vf4_t r0 =
    vf4_mul(vf4_mul(vf4_set(1.f, 1.f, -1.f, 1.f), q0), vf4_wzyx(q1));
  const vf4_t r1 =
    vf4_mul(vf4_mul(vf4_set(-1.f, 1.f, 1.f, 1.f), q0), vf4_zwxy(q1));
  const vf4_t r2 =
    vf4_mul(vf4_mul(vf4_set(1.f, -1.f, 1.f, 1.f), q0), vf4_yxwz(q1));
  const vf4_t r3 =
    vf4_mul(vf4_mul(vf4_set(-1.f, -1.f, -1.f, 1.f), q0), q1);

  const vf4_t ijij = vf4_xayb(vf4_sum(r0), vf4_sum(r1));
  const vf4_t kaka = vf4_xayb(vf4_sum(r2), vf4_sum(r3));
  return vf4_xyab(ijij, kaka);
}

static FINLINE vf4_t
aosq_conj(vf4_t q)  /* { -ix, -jy, -jz, a } */
{
  return vf4_mul(q, vf4_set(-1.f, -1.f, -1.f, 1.f));
}

static FINLINE vf4_t
aosq_calca(vf4_t ijk_)
{
  const vf4_t ijk_square_len = vf4_dot3(ijk_, ijk_);
  return vf4_sqrt(vf4_abs(vf4_sub(vf4_set1(1.f), ijk_square_len)));
}

static FINLINE vf4_t
aosq_nlerp(vf4_t from, vf4_t to, vf4_t aaaa)
{
  return vf4_normalize(vf4_lerp(from, to, aaaa));
}

SIMD_API vf4_t aosq_slerp(vf4_t from, vf4_t to, vf4_t aaaa);

/* Conversion operations. */
SIMD_API void aosq_to_aosf33(vf4_t q, struct aosf33* out);

#endif /* AOSQ_H */


