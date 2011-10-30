#ifndef AOSF44_H
#define AOSF44_H

#include "maths/simd/aosf33.h"
#include "maths/simd/simd.h"
#include "sys/sys.h"
#include <assert.h>

/* Column major float44 data structure. */
struct aosf44 { vf4_t c0, c1, c2, c3; };


/* Set operations. */
static FINLINE void
aosf44_store(float dst[16], const struct aosf44* m)
{
  assert(IS_ALIGNED(dst, 16));
  vf4_store(dst + 0, m->c0);
  vf4_store(dst + 4, m->c1); 
  vf4_store(dst + 8, m->c2); 
  vf4_store(dst + 12, m->c3); 
}

static FINLINE void
aosf44_load(struct aosf44* m, const float src[16])
{
  assert(IS_ALIGNED(src, 16));
  m->c0 = vf4_load(src + 0);
  m->c1 = vf4_load(src + 4);
  m->c2 = vf4_load(src + 8);
  m->c3 = vf4_load(src + 12);
}

static FINLINE void
aosf44_set(struct aosf44* m, vf4_t c0, vf4_t c1, vf4_t c2, vf4_t c3) 
{
  m->c0 = c0;
  m->c1 = c1;
  m->c2 = c2;
  m->c3 = c3;
}

static FINLINE void
aosf44_identity(struct aosf44* m) {
  m->c0 = vf4_set(1.f, 0.f, 0.f, 0.f);
  m->c1 = vf4_set(0.f, 1.f, 0.f, 0.f);
  m->c2 = vf4_set(0.f, 0.f, 1.f, 0.f);
  m->c3 = vf4_set(0.f, 0.f, 0.f, 1.f);
}

static FINLINE void
aosf44_zero(struct aosf44* m) 
{
  m->c0 = vf4_zero();
  m->c1 = vf4_zero();
  m->c2 = vf4_zero();
  m->c3 = vf4_zero();
}

static FINLINE void
aosf44_set_row0(struct aosf44* m, vf4_t v) 
{
  const vf4_t xyzw = v;
  const vf4_t yyww = vf4_yyww(v);
  const vf4_t zwzw = vf4_zwzw(v);
  const vf4_t wwww = vf4_yyww(zwzw);

  m->c0 = vf4_ayzw(m->c0, xyzw);
  m->c1 = vf4_ayzw(m->c1, yyww);
  m->c2 = vf4_ayzw(m->c2, zwzw);
  m->c3 = vf4_ayzw(m->c3, wwww);
}

static FINLINE void
aosf44_set_row1(struct aosf44* m, vf4_t v) 
{
  m->c0 = vf4_xbzw(m->c0, vf4_xxyy(v));
  m->c1 = vf4_xbzw(m->c1, v);
  m->c2 = vf4_xbzw(m->c2, vf4_zzww(v));
  m->c3 = vf4_xbzw(m->c3, vf4_zwzw(v));
}

static FINLINE void
aosf44_set_row2(struct aosf44* m, vf4_t v) 
{
  m->c0 = vf4_xycw(m->c0, vf4_xyxy(v));
  m->c1 = vf4_xycw(m->c1, vf4_xxyy(v));
  m->c2 = vf4_xycw(m->c2, v);
  m->c3 = vf4_xycw(m->c3, vf4_zzww(v));
}

static FINLINE void
aosf44_set_row3(struct aosf44* m, vf4_t v) 
{
  m->c0 = vf4_xyzd(m->c0, vf4_xxxx(v));
  m->c1 = vf4_xyzd(m->c1, vf4_xxyy(v));
  m->c2 = vf4_xyzd(m->c2, vf4_xxzz(v));
  m->c3 = vf4_xyzd(m->c3, v);
}

static FINLINE void
aosf44_set_row(struct aosf44* m, vf4_t v, int id) 
{
  assert(id >= 0 && id <= 3);

  const vf4_t mask = vf4_mask(id == 0, id == 1, id == 2, id == 3);
  m->c0 = vf4_sel(m->c0, vf4_xxxx(v), mask);
  m->c1 = vf4_sel(m->c1, vf4_yyyy(v), mask);
  m->c2 = vf4_sel(m->c2, vf4_zzzz(v), mask);
  m->c3 = vf4_sel(m->c3, vf4_wwww(v), mask);
}

static FINLINE void
aosf44_set_col(struct aosf44* m, vf4_t v, int id)
{
  assert(id >= 0 && id <= 3);
  (&m->c0)[id] = v;
}

/* Get operations. */
static FINLINE vf4_t
aosf44_row0(const struct aosf44* m) 
{
  return vf4_048C
    (vf4_xxxx(m->c0), vf4_xxxx(m->c1), vf4_xxxx(m->c2), vf4_xxxx(m->c3));
}

static FINLINE vf4_t
aosf44_row1(const struct aosf44* m) 
{
  return vf4_048C
    (vf4_yyyy(m->c0), vf4_yyyy(m->c1), vf4_yyyy(m->c2), vf4_yyyy(m->c3));
}

static FINLINE vf4_t
aosf44_row2(const struct aosf44* m) 
{
  return vf4_048C
    (vf4_zzzz(m->c0), vf4_zzzz(m->c1), vf4_zzzz(m->c2), vf4_zzzz(m->c3));
}

static FINLINE vf4_t
aosf44_row3(const struct aosf44* m) 
{
  return vf4_048C
    (vf4_wwww(m->c0), vf4_wwww(m->c1), vf4_wwww(m->c2), vf4_wwww(m->c3));
}

static FINLINE vf4_t
aosf44_row(const struct aosf44* m, int id) 
{
  assert(id >= 0 && id <= 3);

  if(id == 0) {
    return aosf44_row0(m);
  } else if(id == 1) {
    return aosf44_row1(m);
  } else if(id == 2) {
    return aosf44_row2(m);
  } else {
    return aosf44_row3(m);
  }
}

static FINLINE vf4_t
aosf44_col(const struct aosf44* m, int id) 
{
  assert(id >= 0 && id <= 3);
  return (&m->c0)[id];
}

/* Arithmetic operations. */
static FINLINE void
aosf44_add(struct aosf44* res, const struct aosf44* m0, const struct aosf44* m1)
{
  res->c0 = vf4_add(m0->c0, m1->c0);
  res->c1 = vf4_add(m0->c1, m1->c1);
  res->c2 = vf4_add(m0->c2, m1->c2);
  res->c3 = vf4_add(m0->c3, m1->c3);
}

static FINLINE void
aosf44_sub(struct aosf44* res, const struct aosf44* m0, const struct aosf44* m1) 
{
  res->c0 = vf4_sub(m0->c0, m1->c0);
  res->c1 = vf4_sub(m0->c1, m1->c1);
  res->c2 = vf4_sub(m0->c2, m1->c2);
  res->c3 = vf4_sub(m0->c3, m1->c3);
}

static FINLINE void
aosf44_minus(struct aosf44* res, const struct aosf44* m) 
{
  res->c0 = vf4_minus(m->c0);
  res->c1 = vf4_minus(m->c1);
  res->c2 = vf4_minus(m->c2);
  res->c3 = vf4_minus(m->c3);
}

static FINLINE void
aosf44_abs(struct aosf44* res, const struct aosf44* m) 
{
  res->c0 = vf4_abs(m->c0);
  res->c1 = vf4_abs(m->c1);
  res->c2 = vf4_abs(m->c2);
  res->c3 = vf4_abs(m->c3);
}

static FINLINE void
aosf44_mul(struct aosf44* res, const struct aosf44* m, vf4_t v) 
{
  res->c0 = vf4_mul(m->c0, v);
  res->c1 = vf4_mul(m->c1, v);
  res->c2 = vf4_mul(m->c2, v);
  res->c3 = vf4_mul(m->c3, v);
}

static FINLINE vf4_t
aosf44_mulf4(const struct aosf44* m, vf4_t v) 
{
  const vf4_t r0 = vf4_mul(m->c0, vf4_xxxx(v));
  const vf4_t r1 = vf4_madd(m->c1, vf4_yyyy(v), r0);
  const vf4_t r2 = vf4_madd(m->c2, vf4_zzzz(v), r1);
  return vf4_madd(m->c3, vf4_wwww(v), r2);
}

static FINLINE vf4_t
aosf4_mulf44(vf4_t v, const struct aosf44* m) 
{
  const vf4_t xxxx = vf4_dot(v, m->c0); 
  const vf4_t yyyy = vf4_dot(v, m->c1); 
  const vf4_t zzzz = vf4_dot(v, m->c2); 
  const vf4_t wwww = vf4_dot(v, m->c3); 
  const vf4_t xyxy = vf4_xayb(xxxx, yyyy);
  const vf4_t zwzw = vf4_xayb(zzzz, wwww);
  return vf4_xyab(xyxy, zwzw);
}

static FINLINE void
aosf44_mulf44
  (struct aosf44* res, const struct aosf44* m0, const struct aosf44* m1)
{
  const vf4_t c0 = aosf44_mulf4(m0, m1->c0);
  const vf4_t c1 = aosf44_mulf4(m0, m1->c1);
  const vf4_t c2 = aosf44_mulf4(m0, m1->c2);
  const vf4_t c3 = aosf44_mulf4(m0, m1->c3);
  res->c0 = c0;
  res->c1 = c1;
  res->c2 = c2;
  res->c3 = c3;
}

static FINLINE void
aosf44_transpose(struct aosf44* res, const struct aosf44* m) 
{
  const vf4_t in_c0 = m->c0;
  const vf4_t in_c1 = m->c1;
  const vf4_t in_c2 = m->c2;
  const vf4_t in_c3 = m->c3;
  const vf4_t x0x2y0y2 = vf4_xayb(in_c0, in_c2);
  const vf4_t x1x3y1y3 = vf4_xayb(in_c1, in_c3);
  const vf4_t z0z2w0w2 = vf4_zcwd(in_c0, in_c2);
  const vf4_t z1z3w1w3 = vf4_zcwd(in_c1, in_c3);
  res->c0 = vf4_xayb(x0x2y0y2, x1x3y1y3);
  res->c1 = vf4_zcwd(x0x2y0y2, x1x3y1y3);
  res->c2 = vf4_xayb(z0z2w0w2, z1z3w1w3);
  res->c3 = vf4_zcwd(z0z2w0w2, z1z3w1w3);
}

static FINLINE vf4_t
aosf44_det(const struct aosf44* m) 
{
  const struct aosf33 f33_012_012 = { m->c0, m->c1, m->c2 };
  const struct aosf33 f33_012_013 = { m->c0, m->c1, m->c3 };
  const struct aosf33 f33_012_023 = { m->c0, m->c2, m->c3 };
  const struct aosf33 f33_012_123 = { m->c1, m->c2, m->c3 };
  const vf4_t xxxx = vf4_minus(aosf33_det(&f33_012_123));
  const vf4_t yyyy = aosf33_det(&f33_012_023);
  const vf4_t zzzz = vf4_minus(aosf33_det(&f33_012_013));
  const vf4_t wwww = aosf33_det(&f33_012_012);
  const vf4_t xyxy = vf4_xayb(xxxx, yyyy);
  const vf4_t zwzw = vf4_xayb(zzzz, wwww);
  const vf4_t xyzw = vf4_xyab(xyxy, zwzw);
  return vf4_dot(xyzw, aosf44_row3(m));
}

extern vf4_t aosf44_inverse(struct aosf44* out, const struct aosf44* in);

static FINLINE vf4_t
aosf44_invtrans(struct aosf44* out, const struct aosf44* a)
{
  const vf4_t det = aosf44_inverse(out, a);
  aosf44_transpose(out, out);
  return det;
}

#endif /* AOSF44_H */

