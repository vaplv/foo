#ifndef AOSF33_H
#define AOSF33_H

#include "maths/simd/simd.h"
#include "sys/sys.h"
#include <assert.h>

/* Column major float33 data structure. */
struct aosf33 { vf4_t c0, c1, c2; };

/* Set operations. */
static FINLINE void
aosf33_set(struct aosf33* m, vf4_t c0, vf4_t c1, vf4_t c2)
{
  m->c0 = c0;
  m->c1 = c1;
  m->c2 = c2;
}

static FINLINE void
aosf33_identity(struct aosf33* m)
{
  m->c0 = vf4_set(1.f, 0.f, 0.f, 0.f);
  m->c1 = vf4_set(0.f, 1.f, 0.f, 0.f);
  m->c2 = vf4_set(0.f, 0.f, 1.f, 0.f);
}

static FINLINE void
aosf33_zero(struct aosf33* m)
{
  m->c0 = vf4_zero();
  m->c1 = vf4_zero();
  m->c2 = vf4_zero();
}

static FINLINE void
aosf33_set_row0(struct aosf33* m, vf4_t v)
{
  m->c0 = vf4_ayzw(m->c0, v);
  m->c1 = vf4_ayzw(m->c1, vf4_yyww(v));
  m->c2 = vf4_ayzw(m->c2, vf4_zwzw(v));
}

static FINLINE void
aosf33_set_row1(struct aosf33* m, vf4_t v)
{
  m->c0 = vf4_xbzw(m->c0, vf4_xxyy(v));
  m->c1 = vf4_xbzw(m->c1, v);
  m->c2 = vf4_xbzw(m->c2, vf4_zzww(v));
}

static FINLINE void
aosf33_set_row2(struct aosf33* m, vf4_t v)
{
  m->c0 = vf4_xyab(m->c0, vf4_xyxy(v));
  m->c1 = vf4_xyab(m->c1, vf4_yyzz(v));
  m->c2 = vf4_xyab(m->c2, vf4_zzww(v));
}

static FINLINE void
aosf33_set_row(struct aosf33* m, vf4_t v, int id)
{
  assert(id >= 0 && id <= 2);

  const vf4_t mask = vf4_mask(id == 0, id == 1, id == 2, false);
  m->c0 = vf4_sel(m->c0, vf4_xxxx(v), mask);
  m->c1 = vf4_sel(m->c1, vf4_yyyy(v), mask);
  m->c2 = vf4_sel(m->c2, vf4_zzzz(v), mask);
}

static FINLINE void
aosf33_set_col(struct aosf33* m, vf4_t v, int id)
{
  assert(id >= 0 && id <= 2);
  (&m->c0)[id] = v;
}

/* Arithmetic operations. */
static FINLINE void
aosf33_add(struct aosf33* res, const struct aosf33* m0, const struct aosf33* m1)
{
  res->c0 = vf4_add(m0->c0, m1->c0);
  res->c1 = vf4_add(m0->c1, m1->c1);
  res->c2 = vf4_add(m0->c2, m1->c2);
}

static FINLINE void
aosf33_sub(struct aosf33* res, const struct aosf33* m0, const struct aosf33* m1)
{
  res->c0 = vf4_sub(m0->c0, m1->c0);
  res->c1 = vf4_sub(m0->c1, m1->c1);
  res->c2 = vf4_sub(m0->c2, m1->c2);
}

static FINLINE void
aosf33_minus(struct aosf33* res, const struct aosf33* m)
{
  res->c0 = vf4_minus(m->c0);
  res->c1 = vf4_minus(m->c1);
  res->c2 = vf4_minus(m->c2);
}

static FINLINE void
aosf33_abs(struct aosf33* res, const struct aosf33* m)
{
  res->c0 = vf4_abs(m->c0);
  res->c1 = vf4_abs(m->c1);
  res->c2 = vf4_abs(m->c2);
}

static FINLINE void
aosf33_mul(struct aosf33* res, const struct aosf33* m, vf4_t v)
{
  res->c0 = vf4_mul(m->c0, v);
  res->c1 = vf4_mul(m->c1, v);
  res->c2 = vf4_mul(m->c2, v);
}

static FINLINE vf4_t
aosf33_mulf3(const struct aosf33* m, vf4_t v)
{
  const vf4_t r0 = vf4_mul(m->c0, vf4_xxxx(v));
  const vf4_t r1 = vf4_madd(m->c1, vf4_yyyy(v), r0);
  return vf4_madd(m->c2, vf4_zzzz(v), r1);
}

static FINLINE vf4_t
aosf3_mulf33(vf4_t v, const struct aosf33* m)
{
  const vf4_t xxxx = vf4_dot3(v, m->c0);
  const vf4_t yyyy = vf4_dot3(v, m->c1);
  const vf4_t zzzz = vf4_dot3(v, m->c2);
  const vf4_t yyzz = vf4_xyab(yyyy, zzzz);
  return vf4_ayzw(yyzz, xxxx);
}

static FINLINE void
aosf33_mulf33
  (struct aosf33* res, 
   const struct aosf33* a, 
   const struct aosf33* b)
{
  const vf4_t c0 = aosf33_mulf3(a, b->c0);
  const vf4_t c1 = aosf33_mulf3(a, b->c1);
  const vf4_t c2 = aosf33_mulf3(a, b->c2);
  res->c0 = c0;
  res->c1 = c1;
  res->c2 = c2;
}

static FINLINE void
aosf33_transpose(struct aosf33* res, const struct aosf33* m)
{
  const vf4_t c0 = m->c0;
  const vf4_t c1 = m->c1;
  const vf4_t c2 = m->c2;
  const vf4_t x0x2y0y2 = vf4_xayb(c0, c2);
  const vf4_t z0z2w0w2 = vf4_zcwd(c0, c2);
  const vf4_t z1z1y1y1 = vf4_zzyy(c1);
  res->c0 = vf4_xayb(x0x2y0y2, c1);
  res->c1 = vf4_zcwd(x0x2y0y2, z1z1y1y1);
  res->c2 = vf4_xayb(z0z2w0w2, z1z1y1y1);
}

static FINLINE vf4_t
aosf33_det(const struct aosf33* m)
{
  return vf4_dot3(m->c2, vf4_cross3(m->c0, m->c1));
}

static FINLINE vf4_t
aosf33_invtrans(struct aosf33* res, const struct aosf33* m)
{
  const struct aosf33 f33 = {
    vf4_cross3(m->c1, m->c2),
    vf4_cross3(m->c2, m->c0),
    vf4_cross3(m->c0, m->c1)
  };
  const vf4_t det = vf4_dot3(f33.c2, m->c2);
  const vf4_t invdet = vf4_rcp(det);
  aosf33_mul(res, &f33, invdet);
  return det;
}

static FINLINE vf4_t
aosf33_inverse(struct aosf33* res, const struct aosf33* m)
{
  const vf4_t det = aosf33_invtrans(res, m);
  aosf33_transpose(res, res);
  return det;
}

/* Get operations. */
static FINLINE vf4_t
aosf33_row0(const struct aosf33* m)
{
  return vf4_ayzw(vf4_xyab(vf4_xxzz(m->c1), vf4_xxzz(m->c2)), m->c0);
}

static FINLINE vf4_t
aosf33_row1(const struct aosf33* m)
{
  return vf4_ayzw(vf4_xyab(vf4_yyww(m->c1), vf4_yyww(m->c2)), vf4_yyww(m->c0));
}

static FINLINE vf4_t
aosf33_row2(const struct aosf33* m)
{
  return vf4_ayzw(vf4_xyab(vf4_zzww(m->c1), vf4_zzww(m->c2)), vf4_zzww(m->c0));
}

static FINLINE vf4_t
aosf33_row(const struct aosf33* m, int id)
{
  assert(id >= 0 && id <= 2);

  struct aosf33 t;
  aosf33_transpose(&t, m);
  return (&t.c0)[id];
}

static FINLINE vf4_t
aosf33_col(const struct aosf33* m, int id)
{
  assert(id >= 0 && id <= 2);

  return (&m->c0)[id];
}

#endif /* AOSF33_H */

