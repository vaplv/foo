#include "maths/simd/simd.h"
#include "sys/sys.h"

#define KC0 vf4_set1(0.63661977236f)
#define KC1 vf4_set1(1.57079625129f)
#define KC2 vf4_set1(7.54978995489e-8f)
#define CC0 vf4_set1(-0.0013602249f)
#define CC1 vf4_set1(0.0416566950f)
#define CC2 vf4_set1(-0.4999990225f)
#define SC0 vf4_set1(-0.0001950727f)
#define SC1 vf4_set1(0.0083320758f)
#define SC2 vf4_set1(-0.1666665247f)
#define ONE vf4_set1(1.f)

EXPORT_SYM vf4_t
vf4_sin(vf4_t v)
{
  const vi4_t zeroi = vi4_zero();
  const vi4_t onei = vi4_set1(1);
  const vi4_t twoi = vi4_set1(2);
  const vi4_t threei = vi4_set1(3);

  const vf4_t x = vf4_mul(v, KC0);
  const vi4_t q = vf4_to_vi4(x);
  const vi4_t off = vi4_and(q, threei);
  const vf4_t qf = vi4_to_vf4(q);

  const vf4_t tmp = vf4_sub(v, vf4_mul(qf, KC1));
  const vf4_t xl = vf4_sub(tmp, vf4_mul(qf, KC2));
  const vf4_t xl2 = vf4_mul(xl, xl);
  const vf4_t xl3 = vf4_mul(xl2, xl);

  const vf4_t cx =
    vf4_madd(vf4_madd(vf4_madd(CC0, xl2, CC1), xl2, CC2), xl2, ONE);
  const vf4_t sx =
    vf4_madd(vf4_madd(vf4_madd(SC0, xl2, SC1), xl2, SC2), xl3, xl);

  const vf4_t mask0 = (vf4_t) vi4_eq(vi4_and(off, onei), zeroi);
  const vf4_t mask1 = (vf4_t) vi4_eq(vi4_and(off, twoi), zeroi);
  const vf4_t res = vf4_sel(cx, sx, mask0);
  return vf4_sel(vf4_minus(res), res, mask1);
}

EXPORT_SYM vf4_t
vf4_cos(vf4_t v)
{
  const vi4_t zeroi = vi4_zero();
  const vi4_t onei = vi4_set1(1);
  const vi4_t twoi = vi4_set1(2);
  const vi4_t threei = vi4_set1(3);

  const vf4_t x = vf4_mul(v, KC0);
  const vi4_t q = vf4_to_vi4(x);
  const vi4_t off = vi4_add(vi4_and(q, threei), onei);
  const vf4_t qf = vi4_to_vf4(q);

  const vf4_t tmp = vf4_sub(v, vf4_mul(qf, KC1));
  const vf4_t xl = vf4_sub(tmp, vf4_mul(qf, KC2));
  const vf4_t xl2 = vf4_mul(xl, xl);
  const vf4_t xl3 = vf4_mul(xl2, xl);

  const vf4_t cx =
    vf4_madd(vf4_madd(vf4_madd(CC0, xl2, CC1), xl2, CC2), xl2, ONE);
  const vf4_t sx =
    vf4_madd(vf4_madd(vf4_madd(SC0, xl2, SC1), xl2, SC2), xl3, xl);

  const vf4_t mask0 = (vf4_t) vi4_eq(vi4_and(off, onei), zeroi);
  const vf4_t mask1 = (vf4_t) vi4_eq(vi4_and(off, twoi), zeroi);
  const vf4_t res = vf4_sel(cx, sx, mask0);
  return vf4_sel(vf4_minus(res), res, mask1);
}

EXPORT_SYM void
vf4_sincos(vf4_t v, vf4_t* restrict s, vf4_t* restrict c)
{
  const vi4_t zeroi = vi4_zero();
  const vi4_t onei = vi4_set1(1);
  const vi4_t twoi = vi4_set1(2);
  const vi4_t threei = vi4_set1(3);

  const vf4_t x = vf4_mul(v, KC0);
  const vi4_t q = vf4_to_vi4(x);
  const vi4_t soff = vi4_and(q, threei);
  const vi4_t coff = vi4_add(vi4_and(q, threei), onei);
  const vf4_t qf = vi4_to_vf4(q);

  const vf4_t tmp = vf4_sub(v, vf4_mul(qf, KC1));
  const vf4_t xl = vf4_sub(tmp, vf4_mul(qf, KC2));
  const vf4_t xl2 = vf4_mul(xl, xl);
  const vf4_t xl3 = vf4_mul(xl2, xl);

  const vf4_t cx =
    vf4_madd(vf4_madd(vf4_madd(CC0, xl2, CC1), xl2, CC2), xl2, ONE);
  const vf4_t sx =
    vf4_madd(vf4_madd(vf4_madd(SC0, xl2, SC1), xl2, SC2), xl3, xl);

  const vf4_t smask0 = (vf4_t) vi4_eq(vi4_and(soff, onei), zeroi);
  const vf4_t smask1 = (vf4_t) vi4_eq(vi4_and(soff, twoi), zeroi);
  const vf4_t sres = vf4_sel(cx, sx, smask0);
  *s = vf4_sel(vf4_minus(sres), sres, smask1);

  const vf4_t cmask0 = (vf4_t) vi4_eq(vi4_and(coff, onei), zeroi);
  const vf4_t cmask1 = (vf4_t) vi4_eq(vi4_and(coff, twoi), zeroi);
  const vf4_t cres = vf4_sel(cx, sx, cmask0);
  *c = vf4_sel(vf4_minus(cres), cres, cmask1);
}

EXPORT_SYM vf4_t
vf4_acos(vf4_t v)
{
  const vf4_t absv = vf4_abs(v);
  const vf4_t t0 = vf4_sqrt(vf4_sub(vf4_set1(1.f), absv));
  const vf4_t absv2 =vf4_mul(absv, absv);
  const vf4_t absv4 = vf4_mul(absv2, absv2);

  const vf4_t h0 = vf4_set1(-0.0012624911f);
  const vf4_t h1 = vf4_set1(0.0066700901f);
  const vf4_t h2 = vf4_set1(-0.0170881256f);
  const vf4_t h3 = vf4_set1(0.0308918810f);
  const vf4_t hi =
    vf4_madd(vf4_madd(vf4_madd(h0, absv, h1), absv, h2), absv, h3);

  const vf4_t l0 = vf4_set1(-0.0501743046f);
  const vf4_t l1 = vf4_set1(0.0889789874f);
  const vf4_t l2 = vf4_set1(-0.2145988016f);
  const vf4_t l3 = vf4_set1(1.5707963050f);
  const vf4_t lo =
    vf4_madd(vf4_madd(vf4_madd(l0, absv, l1), absv, l2), absv, l3);

  const vf4_t res = vf4_mul(vf4_madd(hi, absv4, lo), t0);
  const vf4_t mask = vf4_lt(v, vf4_zero());

  return vf4_sel(res, vf4_set1(3.14159265358979323846) - res, mask);
}

