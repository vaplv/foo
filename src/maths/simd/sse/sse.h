#ifndef SIMD_SSE_H
#define SIMD_SSE_H

#include "sys/sys.h"
#include <assert.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <stdbool.h>
#include <stdint.h>

typedef __m128 vf4_t;
typedef __m128i vi4_t;

/* Swizzle operands */
#define _SWZ_X 0
#define _SWZ_Y 1
#define _SWZ_Z 2
#define _SWZ_W 3

#define _SIMD_SWZ_VEC4_0(prefix, func, append, op0, op1, op2, op3)\
  static FINLINE prefix##_t\
  prefix##_##func##append(prefix##_t vec)\
  {\
    return _##prefix##_SWZ(vec, op0, op1, op2, op3);\
  }

#define _SIMD_SWZ_VEC4_1(prefix, func, append, op0, op1, op2)\
  _SIMD_SWZ_VEC4_0(prefix, func##append, x, op0, op1, op2, _SWZ_X)\
  _SIMD_SWZ_VEC4_0(prefix, func##append, y, op0, op1, op2, _SWZ_Y)\
  _SIMD_SWZ_VEC4_0(prefix, func##append, z, op0, op1, op2, _SWZ_Z)\
  _SIMD_SWZ_VEC4_0(prefix, func##append, w, op0, op1, op2, _SWZ_W)

#define _SIMD_SWZ_VEC4_2(prefix, func, append, op0, op1)\
  _SIMD_SWZ_VEC4_1(prefix, func##append, x, op0, op1, _SWZ_X)\
  _SIMD_SWZ_VEC4_1(prefix, func##append, y, op0, op1, _SWZ_Y)\
  _SIMD_SWZ_VEC4_1(prefix, func##append, z, op0, op1, _SWZ_Z)\
  _SIMD_SWZ_VEC4_1(prefix, func##append, w, op0, op1, _SWZ_W)

#define _VEC4_GEN_SWZ_FUNCS(prefix)\
  _SIMD_SWZ_VEC4_2(prefix, x, x, _SWZ_X, _SWZ_X)\
  _SIMD_SWZ_VEC4_2(prefix, x, y, _SWZ_X, _SWZ_Y)\
  _SIMD_SWZ_VEC4_2(prefix, x, z, _SWZ_X, _SWZ_Z)\
  _SIMD_SWZ_VEC4_2(prefix, x, w, _SWZ_X, _SWZ_W)\
  _SIMD_SWZ_VEC4_2(prefix, y, x, _SWZ_Y, _SWZ_X)\
  _SIMD_SWZ_VEC4_2(prefix, y, y, _SWZ_Y, _SWZ_Y)\
  _SIMD_SWZ_VEC4_2(prefix, y, z, _SWZ_Y, _SWZ_Z)\
  _SIMD_SWZ_VEC4_2(prefix, y, w, _SWZ_Y, _SWZ_W)\
  _SIMD_SWZ_VEC4_2(prefix, z, x, _SWZ_Z, _SWZ_X)\
  _SIMD_SWZ_VEC4_2(prefix, z, y, _SWZ_Z, _SWZ_Y)\
  _SIMD_SWZ_VEC4_2(prefix, z, z, _SWZ_Z, _SWZ_Z)\
  _SIMD_SWZ_VEC4_2(prefix, z, w, _SWZ_Z, _SWZ_W)\
  _SIMD_SWZ_VEC4_2(prefix, w, x, _SWZ_W, _SWZ_X)\
  _SIMD_SWZ_VEC4_2(prefix, w, y, _SWZ_W, _SWZ_Y)\
  _SIMD_SWZ_VEC4_2(prefix, w, z, _SWZ_W, _SWZ_Z)\
  _SIMD_SWZ_VEC4_2(prefix, w, w, _SWZ_W, _SWZ_W)

/*******************************************************************************
 *
 * 4 packed single precision floating-point values.
 *
 ******************************************************************************/
/* Generate the 256 vec4 swizzle functions (ie: xxxx(), xwwz(), xwyz(), etc.) */
#define _vf4_SWZ(v, a, b, c, d)\
  CHOOSE_EXPR(\
  (a==_SWZ_X)&(b==_SWZ_Y)&(c==_SWZ_Z)&(d==_SWZ_W), v,\
  CHOOSE_EXPR(\
  (a==_SWZ_X)&(b==_SWZ_X)&(c==_SWZ_Y)&(d==_SWZ_Y), _mm_unpacklo_ps(v, v),\
  CHOOSE_EXPR(\
  (a==_SWZ_Z)&(b==_SWZ_Z)&(c==_SWZ_W)&(d==_SWZ_W), _mm_unpackhi_ps(v, v),\
  CHOOSE_EXPR(\
  (a==_SWZ_X)&(b==_SWZ_Y)&(c==_SWZ_X)&(d==_SWZ_Y), _mm_movelh_ps(v, v),\
  CHOOSE_EXPR(\
  (a==_SWZ_Z)&(b==_SWZ_W)&(c==_SWZ_Z)&(d==_SWZ_W), _mm_movehl_ps(v, v),\
  CHOOSE_EXPR(\
  (a==_SWZ_Y)&(b==_SWZ_Y)&(c==_SWZ_W)&(c==_SWZ_W), _mm_movehdup_ps(v),\
  CHOOSE_EXPR(\
  (a==_SWZ_X)&(b==_SWZ_X)&(c==_SWZ_Z)&(c==_SWZ_Z), _mm_moveldup_ps(v),\
  _mm_shuffle_ps(v, v, _MM_SHUFFLE(d, c, b, a))\
  )))))))

#define _VF4_GET_FLOAT(v, i) __builtin_ia32_vec_ext_v4sf(v, i)

/* Swizzle operations. */
/* cppcheck-suppress duplicateExpression */
_VEC4_GEN_SWZ_FUNCS(vf4)

/* Set operations. */
static FINLINE void
vf4_store(float dst[4], vf4_t v)
{
  assert(IS_ALIGNED(dst, 16));
  _mm_store_ps(dst, v);
}

static FINLINE vf4_t
vf4_load(const float dst[4])
{
  assert(IS_ALIGNED(dst, 16));
  return _mm_load_ps(dst);
}

static FINLINE vf4_t
vf4_set1(float x)
{
  return _mm_set1_ps(x);
}

static FINLINE vf4_t
vf4_set(float x, float y, float z, float w)
{
  return _mm_set_ps(w, z, y, x);
}

static FINLINE vf4_t
vf4_zero(void)
{
  return _mm_setzero_ps();
}

static FINLINE vf4_t
vf4_mask(bool x, bool y, bool z, bool w)
{
  const union { float f[4]; int i[4]; } mask = {
    .i[0] = -(x == true),
    .i[1] = -(y == true),
    .i[2] = -(z == true),
    .i[3] = -(w == true)
  };
  return vf4_set(mask.f[0], mask.f[1], mask.f[2], mask.f[3]);
}

static FINLINE vf4_t
vf4_true(void)
{
  const union { float f; int i; } mask = { .i = 0xFFFFFFFF };
  return vf4_set1(mask.f);
}

static FINLINE vf4_t
vf4_false(void)
{
  return vf4_zero();
}

static FINLINE vf4_t
vf4_xmask(void)
{
  const union { float f; int i; } mask = { .i = 0xFFFFFFFF };
  return vf4_set(mask.f, 0.f, 0.f, 0.f);
}

static FINLINE vf4_t
vf4_ymask(void) {
  const union { float f; int i; } mask = { .i = 0xFFFFFFFF };
  return vf4_set(0.f, mask.f, 0.f, 0.f);
}

static FINLINE vf4_t
vf4_zmask(void)
{
  const union { float f; int i; } mask = { .i = 0xFFFFFFFF };
  return vf4_set(0.f, 0.f, mask.f, 0.f);
}

static FINLINE vf4_t
vf4_wmask(void)
{
  const union { float f; int i; } mask = { .i = 0xFFFFFFFF };
  return vf4_set(0.f, 0.f, 0.f, mask.f);
}

/* Extract a float from a SIMD packed representation. */
static FINLINE float
vf4_x(vf4_t v)
{
  return _VF4_GET_FLOAT(v, 0);
}

static FINLINE float
vf4_y(vf4_t v)
{
  return _VF4_GET_FLOAT(v, 1);
}

static FINLINE float
vf4_z(vf4_t v)
{
  return _VF4_GET_FLOAT(v, 2);
}

static FINLINE float
vf4_w(vf4_t v)
{
  return _VF4_GET_FLOAT(v, 3);
}

static FINLINE bool
vf4_mask_x(vf4_t v)
{
  const union { float f; uint32_t ui32; } ucast = { .f = vf4_x(v) };
  return ucast.ui32 != 0;
}

static FINLINE bool
vf4_mask_y(vf4_t v)
{
  const union { float f; uint32_t ui32; } ucast = { .f = vf4_y(v) };
  return ucast.ui32 != 0;
}

static FINLINE bool
vf4_mask_z(vf4_t v)
{
  const union { float f; uint32_t ui32; } ucast = { .f = vf4_z(v) };
  return ucast.ui32 != 0;
}

static FINLINE bool
vf4_mask_w(vf4_t v)
{
  const union { float f; uint32_t ui32; } ucast = { .f = vf4_w(v) };
  return ucast.ui32 != 0;
}

/* Bitwise operations. */
static FINLINE vf4_t
vf4_or(vf4_t v0, vf4_t v1)
{
  return _mm_or_ps(v0, v1);
}

static FINLINE vf4_t
vf4_and(vf4_t v0, vf4_t v1)
{
  return _mm_and_ps(v0, v1);
}

static FINLINE vf4_t
vf4_xor(vf4_t v0, vf4_t v1)
{
  return _mm_xor_ps(v0, v1);
}

static FINLINE vf4_t
vf4_sel(vf4_t vfalse, vf4_t vtrue, vf4_t vcond)
{
  return vf4_xor(vfalse, vf4_and(vcond, vf4_xor(vfalse, vtrue)));
}

/* Merge operations. */
static FINLINE vf4_t
vf4_xayb(vf4_t xyzw, vf4_t abcd)
{
  return _mm_unpacklo_ps(xyzw, abcd);
}

static FINLINE vf4_t
vf4_xyab(vf4_t xyzw, vf4_t abcd)
{
  return _mm_movelh_ps(xyzw, abcd);
}

static FINLINE vf4_t
vf4_zcwd(vf4_t xyzw, vf4_t abcd)
{
  return _mm_unpackhi_ps(xyzw, abcd);
}

static FINLINE vf4_t
vf4_zwcd(vf4_t xyzw, vf4_t abcd)
{
  return _mm_movehl_ps(abcd, xyzw);
}

static FINLINE vf4_t
vf4_ayzw(vf4_t xyzw, vf4_t abcd)
{
  return _mm_move_ss(xyzw, abcd);
}

static FINLINE vf4_t
vf4_xbzw(vf4_t xyzw, vf4_t abcd)
{
  const vf4_t zwzw = _mm_movehl_ps(xyzw, xyzw);
  const vf4_t abzw = _mm_movelh_ps(abcd, zwzw);
  return _mm_move_ss(abzw, xyzw);
}

static FINLINE vf4_t
vf4_xycw(vf4_t xyzw, vf4_t abcd)
{
  const vf4_t yyww = _mm_movehdup_ps(xyzw);
  const vf4_t cwdw = _mm_unpackhi_ps(abcd, yyww);
  return _mm_movelh_ps(xyzw, cwdw);
}

static FINLINE vf4_t
vf4_xyzd(vf4_t xyzw, vf4_t abcd)
{
  const vf4_t bbdd = _mm_movehdup_ps(abcd);
  const vf4_t zdwd = _mm_unpackhi_ps(xyzw, bbdd);
  return _mm_movelh_ps(xyzw, zdwd);
}

static FINLINE vf4_t
vf4_048C(vf4_t _0123, vf4_t _4567, vf4_t _89AB, vf4_t _CDEF)
{
  const vf4_t _0415 = vf4_xayb(_0123, _4567);
  const vf4_t _8C9D = vf4_xayb(_89AB, _CDEF);
  return vf4_xyab(_0415, _8C9D);
}

/* Arithmetic operations. */
static FINLINE vf4_t
vf4_minus(vf4_t v)
{
  return vf4_xor(vf4_set1(-0.f), v);
}

static FINLINE vf4_t
vf4_add(vf4_t v0, vf4_t v1)
{
  return _mm_add_ps(v0, v1);
}

static FINLINE vf4_t
vf4_sub(vf4_t v0, vf4_t v1)
{
  return _mm_sub_ps(v0, v1);
}

static FINLINE vf4_t
vf4_mul(vf4_t v0, vf4_t v1)
{
  return _mm_mul_ps(v0, v1);
}

static FINLINE vf4_t
vf4_div(vf4_t v0, vf4_t v1)
{
  return _mm_div_ps(v0, v1);
}

static FINLINE vf4_t
vf4_madd(vf4_t v0, vf4_t v1, vf4_t v2)
{
  return _mm_add_ps(_mm_mul_ps(v0, v1), v2);
}

static FINLINE vf4_t
vf4_abs(vf4_t v)
{
  const union { float f; int32_t i; } mask = { .i = 0x7fffffff };
  return vf4_and(v, vf4_set1(mask.f));
}

static FINLINE vf4_t
vf4_sqrt(vf4_t v)
{
  return _mm_sqrt_ps(v);
}

static FINLINE vf4_t
vf4_rsqrte(vf4_t v)
{
  return _mm_rsqrt_ps(v);
}

static FINLINE vf4_t
vf4_rsqrt(vf4_t v)
{
  const vf4_t y = vf4_rsqrte(v);
  const vf4_t yyv = vf4_mul(vf4_mul(y, y), v);
  const vf4_t tmp = vf4_sub(vf4_set1(1.5f), vf4_mul(yyv, vf4_set1(0.5f)));
  return vf4_mul(tmp, y);
}

static FINLINE vf4_t
vf4_rcpe(vf4_t v)
{
  return _mm_rcp_ps(v);
}

static FINLINE vf4_t
vf4_rcp(vf4_t v)
{
  const vf4_t y = vf4_rcpe(v);
  const vf4_t tmp = vf4_sub(vf4_set1(2.f), vf4_mul(y, v));
  return vf4_mul(tmp, y);
}

static FINLINE vf4_t
vf4_lerp(vf4_t from, vf4_t to, vf4_t param)
{
  return vf4_madd(vf4_sub(to, from), param, from);
}

static FINLINE vf4_t
vf4_sum(vf4_t v)
{
  const vf4_t r0 = _mm_hadd_ps(v, v);
  return _mm_hadd_ps(r0, r0);
}

static FINLINE vf4_t
vf4_dot(vf4_t v0, vf4_t v1)
{
  return vf4_sum(vf4_mul(v0, v1));
}

static FINLINE vf4_t
vf4_len(vf4_t v)
{
  return vf4_sqrt(vf4_dot(v, v));
}

static FINLINE vf4_t
vf4_normalize(vf4_t v)
{
  return vf4_mul(v, vf4_rsqrt(vf4_dot(v, v)));
}

static FINLINE vf4_t
vf4_sum2(vf4_t v)
{
  return vf4_xxxx(_mm_hadd_ps(v, v));
}

static FINLINE vf4_t
vf4_dot2(vf4_t v0, vf4_t v1)
{
  return vf4_sum2(vf4_mul(v0, v1));
}

static FINLINE vf4_t
vf4_len2(vf4_t v)
{
  return vf4_sqrt(vf4_dot2(v, v));
}

static FINLINE vf4_t
vf4_cross2(vf4_t v0, vf4_t v1)
{
  const vf4_t v = vf4_mul(v0, vf4_yxyx(v1));
  return vf4_sub(vf4_xxxx(v), vf4_yyyy(v));
}

static FINLINE vf4_t
vf4_normalize2(vf4_t v)
{
  return vf4_mul(v, vf4_rsqrt(vf4_dot2(v, v)));
}

static FINLINE vf4_t
vf4_sum3(vf4_t v)
{
  const union { float f; int i; } m = { .i = 0xFFFFFFFF };
  const vf4_t r0 = vf4_and(vf4_set(m.f, m.f, m.f, 0.f), v);
  const vf4_t r1 = _mm_hadd_ps(r0, r0);
  return _mm_hadd_ps(r1, r1);
}

static FINLINE vf4_t
vf4_dot3(vf4_t v0, vf4_t v1)
{
  return vf4_sum3(vf4_mul(v0, v1));
}

static FINLINE vf4_t
vf4_len3(vf4_t v)
{
  return vf4_sqrt(vf4_dot3(v, v));
}

static FINLINE vf4_t
vf4_cross3(vf4_t v0, vf4_t v1)
{
  const vf4_t r0 = vf4_mul(v0, vf4_yzxw(v1));
  const vf4_t r1 = vf4_mul(v1, vf4_yzxw(v0));
  return vf4_yzxw(vf4_sub(r0, r1));
}

static FINLINE vf4_t
vf4_normalize3(vf4_t v)
{
  return vf4_mul(v, vf4_rsqrt(vf4_dot3(v, v)));
}

/* Trigonometric operations. */
extern vf4_t vf4_sin(vf4_t v);
extern vf4_t vf4_cos(vf4_t v);
extern vf4_t vf4_acos(vf4_t v);
extern void vf4_sincos(vf4_t v, vf4_t* restrict s, vf4_t* restrict c);

static FINLINE vf4_t
vf4_tan(vf4_t v)
{
  vf4_t s, c;
  vf4_sincos(v, &s, &c);
  return vf4_div(s, c);
}

static FINLINE vf4_t
vf4_asin(vf4_t v)
{
  return vf4_sub(vf4_set1(1.57079632679489661923f), vf4_acos(v));
}

static FINLINE vf4_t
vf4_atan(vf4_t v)
{
  const vf4_t tmp = vf4_rsqrt(vf4_madd(v, v, vf4_set1(1.f)));
  return vf4_asin(vf4_mul(v, tmp));
}

/* Comparators. */
static FINLINE vf4_t
vf4_eq(vf4_t v0, vf4_t v1)
{
  return _mm_cmpeq_ps(v0, v1);
}

static FINLINE vf4_t
vf4_neq(vf4_t v0, vf4_t v1)
{
  return _mm_cmpneq_ps(v0, v1);
}

static FINLINE vf4_t
vf4_ge(vf4_t v0, vf4_t v1)
{
  return _mm_cmpge_ps(v0, v1);
}

static FINLINE vf4_t
vf4_le(vf4_t v0, vf4_t v1)
{
  return _mm_cmple_ps(v0, v1);
}

static FINLINE vf4_t
vf4_gt(vf4_t v0, vf4_t v1)
{
  return _mm_cmpgt_ps(v0, v1);
}

static FINLINE vf4_t
vf4_lt(vf4_t v0, vf4_t v1)
{
  return _mm_cmplt_ps(v0, v1);
}

static FINLINE vf4_t
vf4_eq_eps(vf4_t v0, vf4_t v1, vf4_t eps)
{
  return vf4_lt(vf4_abs(vf4_sub(v0, v1)), eps);
}

static FINLINE vf4_t
vf4_min(vf4_t v0, vf4_t v1)
{
  return _mm_min_ps(v0, v1);
}

static FINLINE vf4_t
vf4_max(vf4_t v0, vf4_t v1)
{
  return _mm_max_ps(v0, v1);
}

/* Conversion. */
static FINLINE vi4_t
vf4_to_vi4(vf4_t v)
{
  return _mm_cvtps_epi32(v);
}

static FINLINE vf4_t
vi4_to_vf4(vi4_t v)
{
  return _mm_cvtepi32_ps(v);
}

static FINLINE vf4_t /* Cartesian (xyz) to spherical (r, theta, phi)*/
vf4_xyz_to_rthetaphi(vf4_t v)
{
  const vf4_t zero = vf4_zero();
  const vf4_t len2 = vf4_len2(v);
  const vf4_t len3 = vf4_len3(v);
  const vf4_t theta = vf4_sel
    (vf4_acos(vf4_div(vf4_zzzz(v), len3)), zero, vf4_eq(len3, zero));
  const vf4_t tmp_phi = vf4_sel
    (vf4_asin(vf4_div(vf4_yyyy(v), len2)), zero, vf4_eq(len2, zero));
  const vf4_t phi = vf4_sel
    (vf4_sub(vf4_set1(3.14159265358979323846f), tmp_phi), 
     tmp_phi, 
     vf4_ge(vf4_xxxx(v), zero));

  return vf4_xyab(vf4_xayb(len3, theta), phi);
}

#undef _VF4_GET_FLOAT
#undef _vf4_SWZ

/*******************************************************************************
 *
 * 4 packed signed integers.
 *
 ******************************************************************************/
/* Generate the 256 vec4 swizzle functions (ie: xxxx(), xwwz(), xwyz(), etc.) */
#define _vi4_SWZ(v, a, b, c, d)\
  CHOOSE_EXPR(\
  (a==_SWZ_X)&(b==_SWZ_Y)&(c==_SWZ_Z)&(d==_SWZ_W), v,\
  CHOOSE_EXPR(\
  (a==_SWZ_X)&(b==_SWZ_X)&(c==_SWZ_Y)&(d==_SWZ_Y),\
  _mm_unpacklo_epi32(v, v),\
  CHOOSE_EXPR(\
  (a==_SWZ_Z)&(b==_SWZ_Z)&(c==_SWZ_W)&(d==_SWZ_W),\
  _mm_unpackhi_epi32(v, v),\
  _mm_shuffle_epi32(v, _MM_SHUFFLE(d, c, b, a))\
  )))

#define _VI4_GET_INT32(v, i) __builtin_ia32_vec_ext_v4si((__v4si)v, i)

/* Swizzle functions */
/* cppcheck-suppress duplicateExpression */
_VEC4_GEN_SWZ_FUNCS(vi4)

/* Set operations. */
static FINLINE vi4_t
vi4_set1(int32_t i)
{
  return _mm_set1_epi32(i);
}

static FINLINE vi4_t
vi4_set(int32_t x, int32_t y, int32_t z, int32_t w)
{
  return _mm_set_epi32(w, z, y, x);
}

static FINLINE vi4_t
vi4_zero(void) {
  return _mm_setzero_si128();
}

/* Extract int32 from SIMD packed representation. */
static FINLINE int32_t
vi4_x(vi4_t v)
{
  return _VI4_GET_INT32(v, 0);
}

static FINLINE int32_t
vi4_y(vi4_t v)
{
  return _VI4_GET_INT32(v, 1);
}

static FINLINE int32_t
vi4_z(vi4_t v)
{
  return _VI4_GET_INT32(v, 2);
}

static FINLINE int32_t
vi4_w(vi4_t v)
{
  return _VI4_GET_INT32(v, 3);
}

/* Bitwise operators */
static FINLINE vi4_t
vi4_or(vi4_t v0, vi4_t v1)
{
  return _mm_or_si128(v0, v1);
}

static FINLINE vi4_t
vi4_and(vi4_t v0, vi4_t v1)
{
  return _mm_and_si128(v0, v1);
}

static FINLINE vi4_t
vi4_andnot(vi4_t v0, vi4_t v1)
{
  return _mm_andnot_si128(v0, v1);
}

static FINLINE vi4_t
vi4_xor(vi4_t v0, vi4_t v1)
{
  return _mm_xor_si128(v0, v1);
}

static FINLINE vi4_t
vi4_not(vi4_t v)
{
  return _mm_xor_si128(v, _mm_set1_epi32(-1));
}

/* Arithmetic operations.. */
static FINLINE vi4_t
vi4_add(vi4_t v0, vi4_t v1)
{
  return _mm_add_epi32(v0, v1);
}

static FINLINE vi4_t
vi4_sub(vi4_t v0, vi4_t v1)
{
  return _mm_sub_epi32(v0, v1);
}

/* Comparators. */
static FINLINE vi4_t
vi4_eq(vi4_t v0, vi4_t v1)
{
  return _mm_cmpeq_epi32(v0, v1);
}

static FINLINE vi4_t
vi4_neq(vi4_t v0, vi4_t v1)
{
  return vi4_xor(vi4_eq(v0, v1), vi4_set1(-1));
}

static FINLINE vi4_t
vi4_gt(vi4_t v0, vi4_t v1)
{
  return _mm_cmpgt_epi32(v0, v1);
}

static FINLINE vi4_t
vi4_lt(vi4_t v0, vi4_t v1)
{
  return _mm_cmplt_epi32(v0, v1);
}

static FINLINE vi4_t
vi4_ge(vi4_t v0, vi4_t v1)
{
  return vi4_xor(vi4_lt(v0, v1), vi4_set1(-1));
}

static FINLINE vi4_t
vi4_le(vi4_t v0, vi4_t v1)
{
  return vi4_xor(vi4_gt(v0, v1), vi4_set1(-1));
}

static FINLINE vi4_t
vi4_sel(vi4_t vfalse, vi4_t vtrue, vi4_t vcond)
{
  return vi4_xor(vfalse, vi4_and(vcond, vi4_xor(vfalse, vtrue)));
}

#undef _VI4_GET_INT32
#undef _vi4_SWZ

#undef _SIMD_SWZ_VEC4_0
#undef _SIMD_SWZ_VEC4_1
#undef _SIMD_SWZ_VEC4_2
#undef _VEC4_GEN_SWZ_FUNCS
#undef _SWZ_X
#undef _SWZ_Y
#undef _SWZ_Z
#undef _SWZ_W

#endif /* SIMD_SSE_H */

