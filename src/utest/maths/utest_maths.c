#include "maths/simd/aosf33.h"
#include "maths/simd/aosf44.h"
#include "maths/simd/aosq.h"
#include "maths/simd/simd.h"
#include "sys/sys.h"
#include "utest/utest.h"
#include <math.h>

#define PI 3.14159265358979323846
#define EQ_EPS(x, y, eps) (fabsf((x) - (y)) <= (eps))
#define AOSF33_EQ(m, a, b, c, d, e, f, g, h, i) \
  do { \
    CHECK(vf4_x((m).c0), (a)); \
    CHECK(vf4_y((m).c0), (b)); \
    CHECK(vf4_z((m).c0), (c)); \
    CHECK(vf4_x((m).c1), (d)); \
    CHECK(vf4_y((m).c1), (e)); \
    CHECK(vf4_z((m).c1), (f)); \
    CHECK(vf4_x((m).c2), (g)); \
    CHECK(vf4_y((m).c2), (h)); \
    CHECK(vf4_z((m).c2), (i)); \
  } while(0)
#define AOSF33_EQ_EPS(m, a, b, c, d, e, f, g, h, i, eps) \
  do { \
    CHECK(EQ_EPS(vf4_x((m).c0), (a), (eps)), true); \
    CHECK(EQ_EPS(vf4_y((m).c0), (b), (eps)), true); \
    CHECK(EQ_EPS(vf4_z((m).c0), (c), (eps)), true); \
    CHECK(EQ_EPS(vf4_x((m).c1), (d), (eps)), true); \
    CHECK(EQ_EPS(vf4_y((m).c1), (e), (eps)), true); \
    CHECK(EQ_EPS(vf4_z((m).c1), (f), (eps)), true); \
    CHECK(EQ_EPS(vf4_x((m).c2), (g), (eps)), true); \
    CHECK(EQ_EPS(vf4_y((m).c2), (h), (eps)), true); \
    CHECK(EQ_EPS(vf4_z((m).c2), (i), (eps)), true); \
  } while(0)
#define AOSF44_EQ(ma, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
  do { \
    CHECK(vf4_x((ma).c0), (a)); \
    CHECK(vf4_y((ma).c0), (b)); \
    CHECK(vf4_z((ma).c0), (c)); \
    CHECK(vf4_w((ma).c0), (d)); \
    CHECK(vf4_x((ma).c1), (e)); \
    CHECK(vf4_y((ma).c1), (f)); \
    CHECK(vf4_z((ma).c1), (g)); \
    CHECK(vf4_w((ma).c1), (h)); \
    CHECK(vf4_x((ma).c2), (i)); \
    CHECK(vf4_y((ma).c2), (j)); \
    CHECK(vf4_z((ma).c2), (k)); \
    CHECK(vf4_w((ma).c2), (l)); \
    CHECK(vf4_x((ma).c3), (m)); \
    CHECK(vf4_y((ma).c3), (n)); \
    CHECK(vf4_z((ma).c3), (o)); \
    CHECK(vf4_w((ma).c3), (p)); \
  } while(0)
#define AOSF44_EQ_EPS(ma, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, eps) \
  do { \
    CHECK(EQ_EPS(vf4_x((ma).c0), (a), (eps)), true); \
    CHECK(EQ_EPS(vf4_y((ma).c0), (b), (eps)), true); \
    CHECK(EQ_EPS(vf4_z((ma).c0), (c), (eps)), true); \
    CHECK(EQ_EPS(vf4_w((ma).c0), (d), (eps)), true); \
    CHECK(EQ_EPS(vf4_x((ma).c1), (e), (eps)), true); \
    CHECK(EQ_EPS(vf4_y((ma).c1), (f), (eps)), true); \
    CHECK(EQ_EPS(vf4_z((ma).c1), (g), (eps)), true); \
    CHECK(EQ_EPS(vf4_w((ma).c1), (h), (eps)), true); \
    CHECK(EQ_EPS(vf4_x((ma).c2), (i), (eps)), true); \
    CHECK(EQ_EPS(vf4_y((ma).c2), (j), (eps)), true); \
    CHECK(EQ_EPS(vf4_z((ma).c2), (k), (eps)), true); \
    CHECK(EQ_EPS(vf4_w((ma).c2), (l), (eps)), true); \
    CHECK(EQ_EPS(vf4_x((ma).c3), (m), (eps)), true); \
    CHECK(EQ_EPS(vf4_y((ma).c3), (n), (eps)), true); \
    CHECK(EQ_EPS(vf4_z((ma).c3), (o), (eps)), true); \
    CHECK(EQ_EPS(vf4_w((ma).c3), (p), (eps)), true); \
  } while(0)

static void
test_vf4(void)
{
  union {
    int i;
    float f;
  } cast;
  vf4_t i, j, k;
  vi4_t l;
  ALIGN(16) float tmp[4] = { 0.f, 1.f, 2.f, 3.f };

  i = vf4_load(tmp);
  CHECK(vf4_x(i), 0.f);
  CHECK(vf4_y(i), 1.f);
  CHECK(vf4_z(i), 2.f);
  CHECK(vf4_w(i), 3.f);

  tmp[0] = tmp[1] = tmp[2] = tmp[3] = 0.f;
  vf4_store(tmp, i);
  CHECK(tmp[0], 0.f);
  CHECK(tmp[1], 1.f);
  CHECK(tmp[2], 2.f);
  CHECK(tmp[3], 3.f);

  i = vf4_set(1.f, 2.f, 3.f, 4.f);
  CHECK(vf4_x(i), 1.f);
  CHECK(vf4_y(i), 2.f);
  CHECK(vf4_z(i), 3.f);
  CHECK(vf4_w(i), 4.f);

  i = vf4_set1(-2.f);
  CHECK(vf4_x(i), -2.f);
  CHECK(vf4_y(i), -2.f);
  CHECK(vf4_z(i), -2.f);
  CHECK(vf4_w(i), -2.f);

  i = vf4_zero();
  CHECK(vf4_x(i), 0.f);
  CHECK(vf4_y(i), 0.f);
  CHECK(vf4_z(i), 0.f);
  CHECK(vf4_w(i), 0.f);

  i = vf4_mask(true, false, true, true);
  cast.f = vf4_x(i); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_y(i); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(i); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_w(i); CHECK(cast.i, (int)0xFFFFFFFF);

  i = vf4_true();
  cast.f = vf4_x(i); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_y(i); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_z(i); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_w(i); CHECK(cast.i, (int)0xFFFFFFFF);

  i = vf4_false();
  cast.f = vf4_x(i); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(i); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(i); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_w(i); CHECK(cast.i, (int)0x00000000);

  i = vf4_xmask();
  cast.f = vf4_x(i); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_y(i); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(i); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_w(i); CHECK(cast.i, (int)0x00000000);

  i = vf4_ymask();
  cast.f = vf4_x(i); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(i); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_z(i); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_w(i); CHECK(cast.i, (int)0x00000000);

  i = vf4_zmask();
  cast.f = vf4_x(i); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(i); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(i); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_w(i); CHECK(cast.i, (int)0x00000000);

  i = vf4_wmask();
  cast.f = vf4_x(i); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(i); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(i); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_w(i); CHECK(cast.i, (int)0xFFFFFFFF);

  i = vf4_mask(true, false, true, true);
  j = vf4_mask(false, false, false, true);
  k = vf4_or(i, j);
  cast.f = vf4_x(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_y(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_w(k); CHECK(cast.i, (int)0xFFFFFFFF);

  k = vf4_and(i, j);
  cast.f = vf4_x(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_w(k); CHECK(cast.i, (int)0xFFFFFFFF);

  k = vf4_xor(i, j);
  cast.f = vf4_x(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_y(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_w(k); CHECK(cast.i, (int)0x00000000);

  i = vf4_set(1.f, 2.f, 3.f, 4.f);
  j = vf4_set(5.f, 6.f, 7.f, 8.f);
  k = vf4_sel(i, j, vf4_mask(true, false, false, true));
  CHECK(vf4_x(k), 5.f);
  CHECK(vf4_y(k), 2.f);
  CHECK(vf4_z(k), 3.f);
  CHECK(vf4_w(k), 8.f);

  k = vf4_xayb(i, j);
  CHECK(vf4_x(k), 1.f);
  CHECK(vf4_y(k), 5.f);
  CHECK(vf4_z(k), 2.f);
  CHECK(vf4_w(k), 6.f);

  k = vf4_xyab(i, j);
  CHECK(vf4_x(k), 1.f);
  CHECK(vf4_y(k), 2.f);
  CHECK(vf4_z(k), 5.f);
  CHECK(vf4_w(k), 6.f);

  k = vf4_zcwd(i, j);
  CHECK(vf4_x(k), 3.f);
  CHECK(vf4_y(k), 7.f);
  CHECK(vf4_z(k), 4.f);
  CHECK(vf4_w(k), 8.f);

  k = vf4_zwcd(i, j);
  CHECK(vf4_x(k), 3.f);
  CHECK(vf4_y(k), 4.f);
  CHECK(vf4_z(k), 7.f);
  CHECK(vf4_w(k), 8.f);

  k = vf4_ayzw(i, j);
  CHECK(vf4_x(k), 5.f);
  CHECK(vf4_y(k), 2.f);
  CHECK(vf4_z(k), 3.f);
  CHECK(vf4_w(k), 4.f);

  k = vf4_xbzw(i, j);
  CHECK(vf4_x(k), 1.f);
  CHECK(vf4_y(k), 6.f);
  CHECK(vf4_z(k), 3.f);
  CHECK(vf4_w(k), 4.f);

  k = vf4_xycw(i, j);
  CHECK(vf4_x(k), 1.f);
  CHECK(vf4_y(k), 2.f);
  CHECK(vf4_z(k), 7.f);
  CHECK(vf4_w(k), 4.f);

  k = vf4_xyzd(i, j);
  CHECK(vf4_x(k), 1.f);
  CHECK(vf4_y(k), 2.f);
  CHECK(vf4_z(k), 3.f);
  CHECK(vf4_w(k), 8.f);

  k = vf4_048C(vf4_set1(1.f), vf4_set1(2.f), vf4_set1(3.f), vf4_set1(4.f));
  CHECK(vf4_x(k), 1.f);
  CHECK(vf4_y(k), 2.f);
  CHECK(vf4_z(k), 3.f);
  CHECK(vf4_w(k), 4.f);

  i = vf4_set(-1.f, 2.f, -3.f, 4.f);
  j = vf4_minus(i);
  CHECK(vf4_x(j), 1.f);
  CHECK(vf4_y(j), -2.f);
  CHECK(vf4_z(j), 3.f);
  CHECK(vf4_w(j), -4.f);

  k = vf4_add(i, j);
  CHECK(vf4_x(k), 0.f);
  CHECK(vf4_y(k), 0.f);
  CHECK(vf4_z(k), 0.f);
  CHECK(vf4_w(k), 0.f);

  k = vf4_sub(i, j);
  CHECK(vf4_x(k), -2.f);
  CHECK(vf4_y(k), 4.f);
  CHECK(vf4_z(k), -6.f);
  CHECK(vf4_w(k), 8.f);

  k = vf4_mul(i, j);
  CHECK(vf4_x(k), -1.f);
  CHECK(vf4_y(k), -4.f);
  CHECK(vf4_z(k), -9.f);
  CHECK(vf4_w(k), -16.f);

  k = vf4_div(k, i);
  CHECK(vf4_x(k), 1.f);
  CHECK(vf4_y(k), -2.f);
  CHECK(vf4_z(k), 3.f);
  CHECK(vf4_w(k), -4.f);

  k = vf4_madd(i, j, k);
  CHECK(vf4_x(k), 0.f);
  CHECK(vf4_y(k), -6.f);
  CHECK(vf4_z(k), -6.f);
  CHECK(vf4_w(k), -20.f);

  k = vf4_abs(i);
  CHECK(vf4_x(k), 1.f);
  CHECK(vf4_y(k), 2.f);
  CHECK(vf4_z(k), 3.f);
  CHECK(vf4_w(k), 4.f);

  i = vf4_set(4.f, 9.f, 16.f, 25.f);
  k = vf4_sqrt(i);
  CHECK(vf4_x(k), 2.f);
  CHECK(vf4_y(k), 3.f);
  CHECK(vf4_z(k), 4.f);
  CHECK(vf4_w(k), 5.f);

  k = vf4_rsqrte(i);
  CHECK(EQ_EPS(vf4_x(k), 1.f/2.f, 1.e-3f), true);
  CHECK(EQ_EPS(vf4_y(k), 1.f/3.f, 1.e-3f), true);
  CHECK(EQ_EPS(vf4_z(k), 1.f/4.f, 1.e-3f), true);
  CHECK(EQ_EPS(vf4_w(k), 1.f/5.f, 1.e-3f), true);

  k = vf4_rsqrt(i);
  CHECK(EQ_EPS(vf4_x(k), 1.f/2.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), 1.f/3.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), 1.f/4.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(k), 1.f/5.f, 1.e-6f), true);

  k = vf4_rcpe(i);
  CHECK(EQ_EPS(vf4_x(k), 1.f/4.f, 1.e-3f), true);
  CHECK(EQ_EPS(vf4_y(k), 1.f/9.f, 1.e-3f), true);
  CHECK(EQ_EPS(vf4_z(k), 1.f/16.f, 1.e-3f), true);
  CHECK(EQ_EPS(vf4_w(k), 1.f/25.f, 1.e-3f), true);

  k = vf4_rcp(i);
  CHECK(EQ_EPS(vf4_x(k), 1.f/4.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), 1.f/9.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), 1.f/16.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(k), 1.f/25.f, 1.e-6f), true);

	i = vf4_set(0.f, 1.f, 2.f, 4.f);
	j = vf4_set(1.f, 2.f, -1.f, 1.f);
	k = vf4_lerp(i, j, vf4_set1(0.5f));
  CHECK(vf4_x(k), 0.5f);
  CHECK(vf4_y(k), 1.5f);
  CHECK(vf4_z(k), 0.5f);
  CHECK(vf4_w(k), 2.5f);

  k = vf4_sum(j);
  CHECK(vf4_x(k), 3.f);
  CHECK(vf4_y(k), 3.f);
  CHECK(vf4_z(k), 3.f);
  CHECK(vf4_w(k), 3.f);

  k = vf4_dot(i, j);
  CHECK(vf4_x(k), 4.f);
  CHECK(vf4_y(k), 4.f);
  CHECK(vf4_z(k), 4.f);
  CHECK(vf4_w(k), 4.f);

  k = vf4_len(i);
  CHECK(EQ_EPS(vf4_x(k), sqrtf(21.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), sqrtf(21.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), sqrtf(21.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(k), sqrtf(21.f), 1.e-6f), true);

  i = vf4_set(0.f, 4.f, 2.f, 3.f);
  k = vf4_normalize(i);
  CHECK(EQ_EPS(vf4_x(k), 0.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), 0.742781353f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), 0.371390676f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(k), 0.557086014f, 1.e-6f), true);

  i = vf4_set(1.f, 4.f, 2.f, 3.f);
  k = vf4_sum2(i);
  CHECK(vf4_x(k), 5.f);
  CHECK(vf4_y(k), 5.f);
  CHECK(vf4_z(k), 5.f);
  CHECK(vf4_w(k), 5.f);

  j = vf4_set(2.f, 3.f, 5.f, 1.f);
  k = vf4_dot2(i, j);
  CHECK(vf4_x(k), 14.f);
  CHECK(vf4_y(k), 14.f);
  CHECK(vf4_z(k), 14.f);
  CHECK(vf4_w(k), 14.f);

  k = vf4_len2(i);
  CHECK(EQ_EPS(vf4_x(k), sqrtf(17.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), sqrtf(17.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), sqrtf(17.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(k), sqrtf(17.f), 1.e-6f), true);

  i = vf4_set(1.f, -2.f, 2.f, 5.f);
  j = vf4_set(3.f, 1.f, 1.f, 5.f);
  k = vf4_cross2(i, j);
  CHECK(vf4_x(k), 7.f);
  CHECK(vf4_y(k), 7.f);
  CHECK(vf4_z(k), 7.f);
  CHECK(vf4_w(k), 7.f);

  k = vf4_cross2(j, i);
  CHECK(vf4_x(k), -7.f);
  CHECK(vf4_y(k), -7.f);
  CHECK(vf4_z(k), -7.f);
  CHECK(vf4_w(k), -7.f);

  i = vf4_set(0.f, 4.f, 5.f, 7.f);
  k = vf4_normalize2(i);
  CHECK(EQ_EPS(vf4_x(k), 0.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), 1.f, 1.e-6f), true);

  k = vf4_sum3(i);
  CHECK(vf4_x(k), 9.f);
  CHECK(vf4_y(k), 9.f);
  CHECK(vf4_z(k), 9.f);
  CHECK(vf4_w(k), 9.f);

  i = vf4_set(2.f, 3.f, 2.f, 4.f);
  j = vf4_set(0.f, 4.f, 2.f, 19.f);
  k = vf4_dot3(i, j);
  CHECK(vf4_x(k), 16.f);
  CHECK(vf4_y(k), 16.f);
  CHECK(vf4_z(k), 16.f);
  CHECK(vf4_w(k), 16.f);

  k = vf4_len3(j);
  CHECK(EQ_EPS(vf4_x(k), sqrtf(20.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), sqrtf(20.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), sqrtf(20.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(k), sqrtf(20.f), 1.e-6f), true);

  k = vf4_normalize3(j);
  CHECK(EQ_EPS(vf4_x(k), 0.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), 0.8944271910f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), 0.4472135995f, 1.e-6f), true);

  i = vf4_set(1.f, -2.f, 2.f, 4.f);
  j = vf4_set(3.f, 1.f, -1.5f, 2.f);
  k = vf4_cross3(i, j);
  CHECK(vf4_x(k), 1.f);
  CHECK(vf4_y(k), 7.5f);
  CHECK(vf4_z(k), 7.f);

  i = vf4_set(PI/2.f, PI/3.f, PI/4.f, PI/6.f);
  k = vf4_cos(i);
  CHECK(EQ_EPS(vf4_x(k), cosf(PI/2.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), cosf(PI/3.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), cosf(PI/4.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(k), cosf(PI/6.f), 1.e-6f), true);

  k = vf4_sin(i);
  CHECK(EQ_EPS(vf4_x(k), sinf(PI/2.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), sinf(PI/3.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), sinf(PI/4.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(k), sinf(PI/6.f), 1.e-6f), true);

  vf4_sincos(i, &k, &j);
  CHECK(EQ_EPS(vf4_x(k), sinf(PI/2.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), sinf(PI/3.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), sinf(PI/4.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(k), sinf(PI/6.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_x(j), cosf(PI/2.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(j), cosf(PI/3.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(j), cosf(PI/4.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(j), cosf(PI/6.f), 1.e-6f), true);

  i = vf4_set(PI/8.f, PI/3.f, PI/4.f, PI/6.f);
  k = vf4_tan(i);
  CHECK(EQ_EPS(vf4_x(k), tanf(PI/8.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), tanf(PI/3.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), tanf(PI/4.f), 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(k), tanf(PI/6.f), 1.e-6f), true);

  k = vf4_acos(vf4_cos(i));
  CHECK(EQ_EPS(vf4_x(k), PI/8.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), PI/3.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), PI/4.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(k), PI/6.f, 1.e-6f), true);

  k = vf4_asin(vf4_sin(i));
  CHECK(EQ_EPS(vf4_x(k), PI/8.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), PI/3.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), PI/4.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(k), PI/6.f, 1.e-6f), true);

  k = vf4_atan(vf4_tan(i));
  CHECK(EQ_EPS(vf4_x(k), PI/8.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(k), PI/3.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(k), PI/4.f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(k), PI/6.f, 1.e-6f), true);

  i = vf4_set(1.f, 2.f, 3.f, 4.f);
  j = vf4_set(-2.f, -4.f, 3.f, 6.f);
  k = vf4_eq(i, j);
  cast.f = vf4_x(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_w(k); CHECK(cast.i, (int)0x00000000);

  k = vf4_neq(i, j);
  cast.f = vf4_x(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_y(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_z(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_w(k); CHECK(cast.i, (int)0xFFFFFFFF);

  k = vf4_gt(i, j);
  cast.f = vf4_x(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_y(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_z(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_w(k); CHECK(cast.i, (int)0x00000000);

  k = vf4_lt(i, j);
  cast.f = vf4_x(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_w(k); CHECK(cast.i, (int)0xFFFFFFFF);

  k = vf4_ge(i, j);
  cast.f = vf4_x(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_y(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_z(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_w(k); CHECK(cast.i, (int)0x00000000);

  k = vf4_le(i, j);
  cast.f = vf4_x(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_w(k); CHECK(cast.i, (int)0xFFFFFFFF);

  i = vf4_set(1.01f, 2.01f, 3.02f, 0.02f);
  j = vf4_set(1.f, 2.f, 3.f, 0.f);
  k = vf4_set(0.f, 0.01f, 0.02f, 0.f);
  k = vf4_eq_eps(i, j, k);
  cast.f = vf4_x(k); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_z(k); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_w(k); CHECK(cast.i, (int)0x00000000);

  i = vf4_set(1.f, 2.f, 3.f, 4.f);
  j = vf4_set(-2.f, -4.f, 3.f, 6.f);
  k = vf4_min(i, j);
  CHECK(vf4_x(k), -2.f);
  CHECK(vf4_y(k), -4.f);
  CHECK(vf4_z(k), 3.f);
  CHECK(vf4_w(k), 4.f);

  k = vf4_max(i, j);
  CHECK(vf4_x(k), 1.f);
  CHECK(vf4_y(k), 2.f);
  CHECK(vf4_z(k), 3.f);
  CHECK(vf4_w(k), 6.f);

  l = vf4_to_vi4(j);
  CHECK(vi4_x(l), -2);
  CHECK(vi4_y(l), -4);
  CHECK(vi4_z(l), 3);
  CHECK(vi4_w(l), 6);

  k = vi4_to_vf4(l);
  CHECK(vf4_x(k), -2.f);
  CHECK(vf4_y(k), -4.f);
  CHECK(vf4_z(k), 3.f);
  CHECK(vf4_w(k), 6.f);

  k = vf4_xxxx(j);
  CHECK(vf4_x(k), -2.f);
  CHECK(vf4_y(k), -2.f);
  CHECK(vf4_z(k), -2.f);
  CHECK(vf4_w(k), -2.f);

  k = vf4_yyxx(j);
  CHECK(vf4_x(k), -4.f);
  CHECK(vf4_y(k), -4.f);
  CHECK(vf4_z(k), -2.f);
  CHECK(vf4_w(k), -2.f);

  k = vf4_wwxy(j);
  CHECK(vf4_x(k), 6.f);
  CHECK(vf4_y(k), 6.f);
  CHECK(vf4_z(k), -2.f);
  CHECK(vf4_w(k), -4.f);

  k = vf4_zyzy(j);
  CHECK(vf4_x(k), 3.f);
  CHECK(vf4_y(k), -4.f);
  CHECK(vf4_z(k), 3.f);
  CHECK(vf4_w(k), -4.f);

  k = vf4_wyyz(j);
  CHECK(vf4_x(k), 6.f);
  CHECK(vf4_y(k), -4.f);
  CHECK(vf4_z(k), -4.f);
  CHECK(vf4_w(k), 3.f);
}

static void
test_vi4(void)
{
  vi4_t i, j, k;

  i = vi4_set(1, 2, 3, 4);
  CHECK(vi4_x(i), 1);
  CHECK(vi4_y(i), 2);
  CHECK(vi4_z(i), 3);
  CHECK(vi4_w(i), 4);

  i = vi4_set1(-1);
  CHECK(vi4_x(i), -1);
  CHECK(vi4_y(i), -1);
  CHECK(vi4_z(i), -1);
  CHECK(vi4_w(i), -1);

  i = vi4_zero();
  CHECK(vi4_x(i), 0);
  CHECK(vi4_y(i), 0);
  CHECK(vi4_z(i), 0);
  CHECK(vi4_w(i), 0);

  i = vi4_set(0x00010203, 0x04050607, 0x08090A0B, 0x0C0D0E0F);
  j = vi4_set(0x01020401, 0x70605040, 0x0F1F2F3F, 0x00000000);
  k = vi4_or(i, j);
  CHECK(vi4_x(k), (int)0x01030603);
  CHECK(vi4_y(k), (int)0x74655647);
  CHECK(vi4_z(k), (int)0x0F1F2F3F);
  CHECK(vi4_w(k), (int)0x0C0D0E0F);

  k = vi4_and(i, j);
  CHECK(vi4_x(k), (int)0x00000001);
  CHECK(vi4_y(k), (int)0x00000000);
  CHECK(vi4_z(k), (int)0x08090A0B);
  CHECK(vi4_w(k), (int)0x00000000);

  k = vi4_andnot(i, j);
  CHECK(vi4_x(k), (int)0x01020400);
  CHECK(vi4_y(k), (int)0x70605040);
  CHECK(vi4_z(k), (int)0x07162534);
  CHECK(vi4_w(k), (int)0x00000000);

  k = vi4_xor(i, j);
  CHECK(vi4_x(k), (int)0x01030602);
  CHECK(vi4_y(k), (int)0x74655647);
  CHECK(vi4_z(k), (int)0x07162534);
  CHECK(vi4_w(k), (int)0x0C0D0E0F);

  k = vi4_not(i);
  CHECK(vi4_x(k), (int)0xFFFEFDFC);
  CHECK(vi4_y(k), (int)0xFBFAF9F8);
  CHECK(vi4_z(k), (int)0xF7F6F5F4);
  CHECK(vi4_w(k), (int)0xF3F2F1F0);

  i = vi4_set(1, 2, 3, 4);
  j = vi4_set(-2, -4, 3, 6);
  k = vi4_add(i, j);
  CHECK(vi4_x(k), -1);
  CHECK(vi4_y(k), -2);
  CHECK(vi4_z(k), 6);
  CHECK(vi4_w(k), 10);

  k = vi4_sub(i, j);
  CHECK(vi4_x(k), 3);
  CHECK(vi4_y(k), 6);
  CHECK(vi4_z(k), 0);
  CHECK(vi4_w(k), -2);

  k = vi4_eq(i, j);
  CHECK(vi4_x(k), (int)0x00000000);
  CHECK(vi4_y(k), (int)0x00000000);
  CHECK(vi4_z(k), (int)0xFFFFFFFF);
  CHECK(vi4_w(k), (int)0x00000000);

  k = vi4_neq(i, j);
  CHECK(vi4_x(k), (int)0xFFFFFFFF);
  CHECK(vi4_y(k), (int)0xFFFFFFFF);
  CHECK(vi4_z(k), (int)0x00000000);
  CHECK(vi4_w(k), (int)0xFFFFFFFF);

  k = vi4_gt(i, j);
  CHECK(vi4_x(k), (int)0xFFFFFFFF);
  CHECK(vi4_y(k), (int)0xFFFFFFFF);
  CHECK(vi4_z(k), (int)0x00000000);
  CHECK(vi4_w(k), (int)0x00000000);

  k = vi4_lt(i, j);
  CHECK(vi4_x(k), (int)0x00000000);
  CHECK(vi4_y(k), (int)0x00000000);
  CHECK(vi4_z(k), (int)0x00000000);
  CHECK(vi4_w(k), (int)0xFFFFFFFF);

  k = vi4_ge(i, j);
  CHECK(vi4_x(k), (int)0xFFFFFFFF);
  CHECK(vi4_y(k), (int)0xFFFFFFFF);
  CHECK(vi4_z(k), (int)0xFFFFFFFF);
  CHECK(vi4_w(k), (int)0x00000000);

  k = vi4_le(i, j);
  CHECK(vi4_x(k), (int)0x00000000);
  CHECK(vi4_y(k), (int)0x00000000);
  CHECK(vi4_z(k), (int)0xFFFFFFFF);
  CHECK(vi4_w(k), (int)0xFFFFFFFF);

  k = vi4_sel(i, j, vi4_set(0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000));
  CHECK(vi4_x(k), -2);
  CHECK(vi4_y(k), 2);
  CHECK(vi4_z(k), 3);
  CHECK(vi4_w(k), 4);

  k = vi4_xxxx(i);
  CHECK(vi4_x(k), 1);
  CHECK(vi4_y(k), 1);
  CHECK(vi4_z(k), 1);
  CHECK(vi4_w(k), 1);

  k = vi4_wwxy(i);
  CHECK(vi4_x(k), 4);
  CHECK(vi4_y(k), 4);
  CHECK(vi4_z(k), 1);
  CHECK(vi4_w(k), 2);

  k = vi4_xyxy(i);
  CHECK(vi4_x(k), 1);
  CHECK(vi4_y(k), 2);
  CHECK(vi4_z(k), 1);
  CHECK(vi4_w(k), 2);

  k = vi4_wyyz(i);
  CHECK(vi4_x(k), 4);
  CHECK(vi4_y(k), 2);
  CHECK(vi4_z(k), 2);
  CHECK(vi4_w(k), 3);
}

static void
test_aosf33(void)
{
  vf4_t v;
  struct aosf33 m, n, o;

  aosf33_set
    (&m,
     vf4_set(0.f, 1.f, 2.f, 0.f),
     vf4_set(3.f, 4.f, 5.f, 0.f),
     vf4_set(6.f, 7.f, 8.f, 0.f));
  AOSF33_EQ(m, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f);
  aosf33_identity(&m);
  AOSF33_EQ(m, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f);
  aosf33_zero(&m);
  AOSF33_EQ(m, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);

  aosf33_set_row0(&m, vf4_set(0.f, 1.f, 2.f, 9.f));
  AOSF33_EQ(m, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 2.f, 0.f, 0.f);
  aosf33_set_row1(&m, vf4_set(3.f, 4.f, 5.f, 10.f));
  AOSF33_EQ(m, 0.f, 3.f, 0.f, 1.f, 4.f, 0.f, 2.f, 5.f, 0.f);
  aosf33_set_row2(&m, vf4_set(6.f, 7.f, 8.f, 11.f));
  AOSF33_EQ(m, 0.f, 3.f, 6.f, 1.f, 4.f, 7.f, 2.f, 5.f, 8.f);

  aosf33_zero(&m);
  aosf33_set_row(&m, vf4_set(0.f, 1.f, 2.f, 9.f), 0);
  AOSF33_EQ(m, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 2.f, 0.f, 0.f);
  aosf33_set_row(&m, vf4_set(3.f, 4.f, 5.f, 10.f), 1);
  AOSF33_EQ(m, 0.f, 3.f, 0.f, 1.f, 4.f, 0.f, 2.f, 5.f, 0.f);
  aosf33_set_row(&m, vf4_set(6.f, 7.f, 8.f, 11.f), 2);
  AOSF33_EQ(m, 0.f, 3.f, 6.f, 1.f, 4.f, 7.f, 2.f, 5.f, 8.f);

  aosf33_zero(&m);
  aosf33_set_col(&m, vf4_set(0.f, 1.f, 2.f, 9.f), 0);
  AOSF33_EQ(m, 0.f, 1.f, 2.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
  aosf33_set_col(&m, vf4_set(3.f, 4.f, 5.f, 10.f), 1);
  AOSF33_EQ(m, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 0.f, 0.f, 0.f);
  aosf33_set_col(&m, vf4_set(6.f, 7.f, 8.f, 11.f), 2);
  AOSF33_EQ(m, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f);

  v = aosf33_row0(&m);
  CHECK(vf4_x(v), 0.f);
  CHECK(vf4_y(v), 3.f);
  CHECK(vf4_z(v), 6.f);

  v = aosf33_row1(&m);
  CHECK(vf4_x(v), 1.f);
  CHECK(vf4_y(v), 4.f);
  CHECK(vf4_z(v), 7.f);

  v = aosf33_row2(&m);
  CHECK(vf4_x(v), 2.f);
  CHECK(vf4_y(v), 5.f);
  CHECK(vf4_z(v), 8.f);

  v = aosf33_row(&m, 0);
  CHECK(vf4_x(v), 0.f);
  CHECK(vf4_y(v), 3.f);
  CHECK(vf4_z(v), 6.f);

  v = aosf33_row(&m, 1);
  CHECK(vf4_x(v), 1.f);
  CHECK(vf4_y(v), 4.f);
  CHECK(vf4_z(v), 7.f);

  v = aosf33_row(&m, 2);
  CHECK(vf4_x(v), 2.f);
  CHECK(vf4_y(v), 5.f);
  CHECK(vf4_z(v), 8.f);

  v = aosf33_col(&m, 0);
  CHECK(vf4_x(v), 0.f);
  CHECK(vf4_y(v), 1.f);
  CHECK(vf4_z(v), 2.f);

  v = aosf33_col(&m, 1);
  CHECK(vf4_x(v), 3.f);
  CHECK(vf4_y(v), 4.f);
  CHECK(vf4_z(v), 5.f);

  v = aosf33_col(&m, 2);
  CHECK(vf4_x(v), 6.f);
  CHECK(vf4_y(v), 7.f);
  CHECK(vf4_z(v), 8.f);

  aosf33_set
    (&m,
     vf4_set(0.f, 1.f, 2.f, 0.f),
     vf4_set(3.f, 4.f, 5.f, 0.f),
     vf4_set(6.f, 7.f, 8.f, 0.f));
  aosf33_set
    (&n,
     vf4_set(1.f, 2.f, 3.f, 0.f),
     vf4_set(4.f, 5.f, 6.f, 0.f),
     vf4_set(7.f, 8.f, 9.f, 0.f));
  aosf33_add(&o, &m, &n);
  AOSF33_EQ(o, 1.f, 3.f, 5.f, 7.f, 9.f, 11.f, 13.f, 15.f, 17.f);
  aosf33_sub(&o, &o, &n);
	AOSF33_EQ(o, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f);

  aosf33_set
    (&m,
     vf4_set(1.f, 2.f, -3.f, 0.f),
     vf4_set(-4.f, -5.f, 6.f, 0.f),
     vf4_set(7.f, -8.f, 9.f, 0.f));
  aosf33_minus(&m, &m);
	AOSF33_EQ(m, -1.f, -2.f, 3.f, 4.f, 5.f, -6.f, -7.f, 8.f, -9.f);

  aosf33_mul(&o, &m, vf4_set1(2.f));
	AOSF33_EQ(o, -2.f, -4.f, 6.f, 8.f, 10.f, -12.f, -14.f, 16.f, -18.f);

  aosf33_set
    (&m,
     vf4_set(1.f, 2.f, 3.f, 0.f),
     vf4_set(4.f, 5.f, 6.f, 0.f),
     vf4_set(7.f, 8.f, 9.f, 0.f));
  v = aosf33_mulf3(&m, vf4_set(1.f, 2.f, 3.f, 0.f));
  CHECK(vf4_x(v), 30.f);
  CHECK(vf4_y(v), 36.f);
  CHECK(vf4_z(v), 42.f);
  v = aosf3_mulf33(vf4_set(1.f, 2.f, 3.f, 0.f), &m);
  CHECK(vf4_x(v), 14.f);
  CHECK(vf4_y(v), 32.f);
  CHECK(vf4_z(v), 50.f);
  aosf33_set
    (&n,
     vf4_set(2.f, 9.f, 8.f, 0.f),
     vf4_set(1.f, -2.f, 2.f, 0.f),
     vf4_set(1.f, -8.f, -4.f, 0.f));
  aosf33_mulf33(&o, &m, &n);
	AOSF33_EQ(o, 94.f, 113.f, 132.f, 7.f, 8.f, 9.f, -59.f, -70.f, -81.f);

  aosf33_transpose(&o, &m);
	AOSF33_EQ(o, 1.f, 4.f, 7.f, 2.f, 5.f, 8.f, 3.f, 6.f, 9.f);

  aosf33_set
    (&m,
     vf4_set(1.f, 2.f, 3.f, 0.f),
     vf4_set(4.f, 5.f, 6.f, 0.f),
     vf4_set(3.f, -4.f, 9.f, 0.f));
  v = aosf33_det(&m);
  CHECK(vf4_x(v), -60.f);
  CHECK(vf4_y(v), -60.f);
  CHECK(vf4_z(v), -60.f);
  CHECK(vf4_w(v), -60.f);

  v = aosf33_inverse(&n, &m);
  CHECK(vf4_x(v), -60.f);
  CHECK(vf4_y(v), -60.f);
  CHECK(vf4_z(v), -60.f);
  CHECK(vf4_w(v), -60.f);
  aosf33_mulf33(&o, &m, &n);
  AOSF33_EQ_EPS(o, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.e-6f);

  v = aosf33_invtrans(&o, &m);
  CHECK(vf4_x(v), -60.f);
  CHECK(vf4_y(v), -60.f);
  CHECK(vf4_z(v), -60.f);
  CHECK(vf4_w(v), -60.f);
  AOSF33_EQ
    (o,
     vf4_x(n.c0), vf4_x(n.c1), vf4_x(n.c2),
     vf4_y(n.c0), vf4_y(n.c1), vf4_y(n.c2),
     vf4_z(n.c0), vf4_z(n.c1), vf4_z(n.c2));
}

static void
test_aosf44(void)
{
  vf4_t v;
  struct aosf44 m, n, o;
  ALIGN(16) float tmp[16];

  aosf44_set
    (&m,
     vf4_set(0.f, 1.f, 2.f, 3.f),
     vf4_set(4.f, 5.f, 6.f, 7.f),
     vf4_set(8.f, 9.f, 10.f, 11.f),
     vf4_set(12.f, 13.f, 14.f, 15.f));
  AOSF44_EQ
    (m,
     0.f, 1.f, 2.f, 3.f,
     4.f, 5.f, 6.f, 7.f,
     8.f, 9.f, 10.f, 11.f,
     12.f, 13.f, 14.f, 15.f);

  aosf44_store(tmp, &m);
  CHECK(tmp[0], 0.f);
  CHECK(tmp[1], 1.f);
  CHECK(tmp[2], 2.f);
  CHECK(tmp[3], 3.f);
  CHECK(tmp[4], 4.f);
  CHECK(tmp[5], 5.f);
  CHECK(tmp[6], 6.f);
  CHECK(tmp[7], 7.f);
  CHECK(tmp[8], 8.f);
  CHECK(tmp[9], 9.f);
  CHECK(tmp[10], 10.f);
  CHECK(tmp[11], 11.f);
  CHECK(tmp[12], 12.f);
  CHECK(tmp[13], 13.f);
  CHECK(tmp[14], 14.f);
  CHECK(tmp[15], 15.f);

  tmp[0] = 0.f; tmp[1] = 2.f; tmp[2] = 4.f; tmp[3] = 6.f;
  tmp[4] = 8.f; tmp[5] = 10.f; tmp[6] = 12.f; tmp[7] = 14.f;
  tmp[8] = 16.f; tmp[9] = 18.f; tmp[10] = 20.f; tmp[11] = 22.f;
  tmp[12] = 24.f; tmp[13] = 26.f; tmp[14] = 28.f; tmp[15] = 30.f;
  aosf44_load(&m, tmp);
  AOSF44_EQ
    (m,
     0.f, 2.f, 4.f, 6.f,
     8.f, 10.f, 12.f, 14.f,
     16.f, 18.f, 20.f, 22.f,
     24.f, 26.f, 28.f, 30.f);

  aosf44_identity(&m);
  AOSF44_EQ
    (m,
     1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     0.f, 0.f, 0.f, 1.f);

  aosf44_zero(&m);
  AOSF44_EQ
    (m,
     0.f, 0.f, 0.f, 0.f,
     0.f, 0.f, 0.f, 0.f,
     0.f, 0.f, 0.f, 0.f,
     0.f, 0.f, 0.f, 0.f);

  aosf44_set_row0(&m, vf4_set(0.f, 1.f, 2.f, 3.f));
  AOSF44_EQ
    (m,
     0.f, 0.f, 0.f, 0.f,
     1.f, 0.f, 0.f, 0.f,
     2.f, 0.f, 0.f, 0.f,
     3.f, 0.f, 0.f, 0.f);
  aosf44_set_row1(&m, vf4_set(4.f, 5.f, 6.f, 7.f));
  AOSF44_EQ
    (m,
     0.f, 4.f, 0.f, 0.f,
     1.f, 5.f, 0.f, 0.f,
     2.f, 6.f, 0.f, 0.f,
     3.f, 7.f, 0.f, 0.f);
  aosf44_set_row2(&m, vf4_set(8.f, 9.f, 10.f, 11.f));
  AOSF44_EQ
    (m,
     0.f, 4.f, 8.f, 0.f,
     1.f, 5.f, 9.f, 0.f,
     2.f, 6.f, 10.f, 0.f,
     3.f, 7.f, 11.f, 0.f);
  aosf44_set_row3(&m, vf4_set(12.f, 13.f, 14.f, 15.f));
  AOSF44_EQ
    (m,
     0.f, 4.f, 8.f, 12.f,
     1.f, 5.f, 9.f, 13.f,
     2.f, 6.f, 10.f, 14.f,
     3.f, 7.f, 11.f, 15.f);

  aosf44_zero(&m);
  aosf44_set_row(&m, vf4_set(0.f, 1.f, 2.f, 3.f), 0);
  AOSF44_EQ
    (m,
     0.f, 0.f, 0.f, 0.f,
     1.f, 0.f, 0.f, 0.f,
     2.f, 0.f, 0.f, 0.f,
     3.f, 0.f, 0.f, 0.f);
  aosf44_set_row(&m, vf4_set(4.f, 5.f, 6.f, 7.f), 1);
  AOSF44_EQ
    (m,
     0.f, 4.f, 0.f, 0.f,
     1.f, 5.f, 0.f, 0.f,
     2.f, 6.f, 0.f, 0.f,
     3.f, 7.f, 0.f, 0.f);
  aosf44_set_row(&m, vf4_set(8.f, 9.f, 10.f, 11.f), 2);
  AOSF44_EQ
    (m,
     0.f, 4.f, 8.f, 0.f,
     1.f, 5.f, 9.f, 0.f,
     2.f, 6.f, 10.f, 0.f,
     3.f, 7.f, 11.f, 0.f);
  aosf44_set_row(&m, vf4_set(12.f, 13.f, 14.f, 15.f), 3);
  AOSF44_EQ
    (m,
     0.f, 4.f, 8.f, 12.f,
     1.f, 5.f, 9.f, 13.f,
     2.f, 6.f, 10.f, 14.f,
     3.f, 7.f, 11.f, 15.f);

  aosf44_zero(&m);
  aosf44_set_col(&m, vf4_set(0.f, 1.f, 2.f, 3.f), 0);
  AOSF44_EQ
    (m,
     0.f, 1.f, 2.f, 3.f,
     0.f, 0.f, 0.f, 0.f,
     0.f, 0.f, 0.f, 0.f,
     0.f, 0.f, 0.f, 0.f);
  aosf44_set_col(&m, vf4_set(4.f, 5.f, 6.f, 7.f), 1);
  AOSF44_EQ
    (m,
     0.f, 1.f, 2.f, 3.f,
     4.f, 5.f, 6.f, 7.f,
     0.f, 0.f, 0.f, 0.f,
     0.f, 0.f, 0.f, 0.f);
 aosf44_set_col(&m, vf4_set(8.f, 9.f, 10.f, 11.f), 2);
  AOSF44_EQ
    (m,
     0.f, 1.f, 2.f, 3.f,
     4.f, 5.f, 6.f, 7.f,
     8.f, 9.f, 10.f, 11.f,
     0.f, 0.f, 0.f, 0.f);
  aosf44_set_col(&m, vf4_set(12.f, 13.f, 14.f, 15.f), 3);
  AOSF44_EQ
    (m,
     0.f, 1.f, 2.f, 3.f,
     4.f, 5.f, 6.f, 7.f,
     8.f, 9.f, 10.f, 11.f,
     12.f, 13.f, 14.f, 15.f);

  v = aosf44_row0(&m);
  CHECK(vf4_x(v), 0.f);
  CHECK(vf4_y(v), 4.f);
  CHECK(vf4_z(v), 8.f);
  CHECK(vf4_w(v), 12.f);

  v = aosf44_row1(&m);
  CHECK(vf4_x(v), 1.f);
  CHECK(vf4_y(v), 5.f);
  CHECK(vf4_z(v), 9.f);
  CHECK(vf4_w(v), 13.f);

  v = aosf44_row2(&m);
  CHECK(vf4_x(v), 2.f);
  CHECK(vf4_y(v), 6.f);
  CHECK(vf4_z(v), 10.f);
  CHECK(vf4_w(v), 14.f);

  v = aosf44_row3(&m);
  CHECK(vf4_x(v), 3.f);
  CHECK(vf4_y(v), 7.f);
  CHECK(vf4_z(v), 11.f);
  CHECK(vf4_w(v), 15.f);

  v = aosf44_row(&m, 0);
  CHECK(vf4_x(v), 0.f);
  CHECK(vf4_y(v), 4.f);
  CHECK(vf4_z(v), 8.f);
  CHECK(vf4_w(v), 12.f);

  v = aosf44_row(&m, 1);
  CHECK(vf4_x(v), 1.f);
  CHECK(vf4_y(v), 5.f);
  CHECK(vf4_z(v), 9.f);
  CHECK(vf4_w(v), 13.f);

  v = aosf44_row(&m, 2);
  CHECK(vf4_x(v), 2.f);
  CHECK(vf4_y(v), 6.f);
  CHECK(vf4_z(v), 10.f);
  CHECK(vf4_w(v), 14.f);

  v = aosf44_row(&m, 3);
  CHECK(vf4_x(v), 3.f);
  CHECK(vf4_y(v), 7.f);
  CHECK(vf4_z(v), 11.f);
  CHECK(vf4_w(v), 15.f);

  v = aosf44_col(&m, 0);
  CHECK(vf4_x(v), 0.f);
  CHECK(vf4_y(v), 1.f);
  CHECK(vf4_z(v), 2.f);
  CHECK(vf4_w(v), 3.f);

  v = aosf44_col(&m, 1);
  CHECK(vf4_x(v), 4.f);
  CHECK(vf4_y(v), 5.f);
  CHECK(vf4_z(v), 6.f);
  CHECK(vf4_w(v), 7.f);

  v = aosf44_col(&m, 2);
  CHECK(vf4_x(v), 8.f);
  CHECK(vf4_y(v), 9.f);
  CHECK(vf4_z(v), 10.f);
  CHECK(vf4_w(v), 11.f);

  v = aosf44_col(&m, 3);
  CHECK(vf4_x(v), 12.f);
  CHECK(vf4_y(v), 13.f);
  CHECK(vf4_z(v), 14.f);
  CHECK(vf4_w(v), 15.f);

  aosf44_set
    (&m,
     vf4_set(0.f, 1.f, 2.f, 3.f),
     vf4_set(4.f, 5.f, 6.f, 7.f),
     vf4_set(8.f, 9.f, 10.f, 11.f),
     vf4_set(12.f, 13.f, 14.f, 15.f));
  aosf44_set
    (&n,
     vf4_set(0.f, 2.f, 1.f, 3.f),
     vf4_set(1.f, -2.f, -1.f, -3.f),
     vf4_set(1.f, 0.f, 0.f, 2.f),
     vf4_set(3.f, 2.f, 1.f, 0.f));
  aosf44_add(&o, &m, &n);
  AOSF44_EQ
    (o,
     0.f, 3.f, 3.f, 6.f,
     5.f, 3.f, 5.f, 4.f,
     9.f, 9.f, 10.f, 13.f,
     15.f, 15.f, 15.f, 15.f);

  aosf44_sub(&o, &m, &n);
  AOSF44_EQ
    (o,
     0.f, -1.f, 1.f, 0.f,
     3.f, 7.f, 7.f, 10.f,
     7.f, 9.f, 10.f, 9.f,
     9.f, 11.f, 13.f, 15.f);

  aosf44_minus(&o, &n);
  AOSF44_EQ
    (o,
     0.f, -2.f, -1.f, -3.f,
     -1.f, 2.f, 1.f, 3.f,
     -1.f, 0.f, 0.f, -2.f,
     -3.f, -2.f, -1.f, 0.f);

  aosf44_abs(&o, &o);
  AOSF44_EQ
    (o,
     0.f, 2.f, 1.f, 3.f,
     1.f, 2.f, 1.f, 3.f,
     1.f, 0.f, 0.f, 2.f,
     3.f, 2.f, 1.f, 0.f);

  aosf44_mul(&o, &n, vf4_set(1.f, 2.f, 3.f, 2.f));
  AOSF44_EQ
    (o,
     0.f, 4.f, 3.f, 6.f,
     1.f, -4.f, -3.f, -6.f,
     1.f, 0.f, 0.f, 4.f,
     3.f, 4.f, 3.f, 0.f);

  aosf44_set
    (&m,
     vf4_set(0.f, 1.f, 2.f, 3.f),
     vf4_set(4.f, 5.f, 6.f, 7.f),
     vf4_set(8.f, 9.f, 10.f, 11.f),
     vf4_set(12.f, 13.f, 14.f, 15.f));
  v = aosf44_mulf4(&m, vf4_set(1.f, 2.f, 3.f, 1.f));
  CHECK(vf4_x(v), 44.f);
  CHECK(vf4_y(v), 51.f);
  CHECK(vf4_z(v), 58.f);
  CHECK(vf4_w(v), 65.f);

  v = aosf4_mulf44(vf4_set(1.f, 2.f, 3.f, 1.f), &m);
  CHECK(vf4_x(v), 11.f);
  CHECK(vf4_y(v), 39.f);
  CHECK(vf4_z(v), 67.f);
  CHECK(vf4_w(v), 95.f);

  aosf44_set
    (&m,
     vf4_set(1.f, 2.f, 3.f, 4.f),
     vf4_set(4.f, 5.f, 6.f, 7.f),
     vf4_set(7.f, 8.f, 9.f, 10.f),
     vf4_set(10.f, 11.f, 12.f, 13.f));
  aosf44_set
    (&n,
     vf4_set(2.f, 9.f, 8.f, 1.f),
     vf4_set(1.f, -2.f, 2.f, 1.f),
     vf4_set(1.f, -8.f, -4.f, 2.f),
     vf4_set(1.f, 3.f, 4.f, 2.f));
  aosf44_mulf44(&o, &m, &n);
  AOSF44_EQ
    (o,
     104.f, 124.f, 144.f, 164.f,
     17.f, 19.f, 21.f, 23.f,
     -39.f, -48.f, -57.f, -66.f,
     61.f, 71.f, 81.f, 91.f);

  aosf44_transpose(&o, &n);
  AOSF44_EQ
    (o,
     2.f, 1.f, 1.f, 1.f,
     9.f, -2.f, -8.f, 3.f,
     8.f, 2.f, -4.f, 4.f,
     1.f, 1.f, 2.f, 2.f);

  v = aosf44_det(&n);
  CHECK(vf4_x(v), 78.f);
  CHECK(vf4_y(v), 78.f);
  CHECK(vf4_z(v), 78.f);
  CHECK(vf4_w(v), 78.f);

  v = aosf44_inverse(&m, &n);
  CHECK(vf4_x(v), 78.f);
  CHECK(vf4_y(v), 78.f);
  CHECK(vf4_z(v), 78.f);
  CHECK(vf4_w(v), 78.f);
  aosf44_mulf44(&o, &m, &n);
  AOSF44_EQ_EPS
    (o,
     1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     0.f, 0.f, 0.f, 1.f,
     1.e-6f);

  v = aosf44_invtrans(&o, &n);
  CHECK(vf4_x(v), 78.f);
  CHECK(vf4_y(v), 78.f);
  CHECK(vf4_z(v), 78.f);
  CHECK(vf4_w(v), 78.f);
  AOSF44_EQ
    (o,
     vf4_x(m.c0), vf4_x(m.c1), vf4_x(m.c2), vf4_x(m.c3),
     vf4_y(m.c0), vf4_y(m.c1), vf4_y(m.c2), vf4_y(m.c3),
     vf4_z(m.c0), vf4_z(m.c1), vf4_z(m.c2), vf4_z(m.c3),
     vf4_w(m.c0), vf4_w(m.c1), vf4_w(m.c2), vf4_w(m.c3));
}

static void
test_aosq(void)
{
  union {
    int i;
    float f;
  } cast;
  vf4_t q0, q1, q2, t;
  struct aosf33 m;

  q0 = aosq_identity();
  CHECK(vf4_x(q0), 0.f);
  CHECK(vf4_y(q0), 0.f);
  CHECK(vf4_z(q0), 0.f);
  CHECK(vf4_w(q0), 1.f);

  q0 = aosq_set_axis_angle(vf4_set(2.f, 5.f, 1.f, 0.f), vf4_set1(PI*0.3f));
  CHECK(EQ_EPS(vf4_x(q0), 0.907981f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(q0), 2.269953f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(q0), 0.453991f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(q0), 0.891007f, 1.e-6f), true);

  q0 = vf4_set(1.f, 2.f, 3.f, -3.f);
  q1 = vf4_set(1.f, 2.f, 3.f, -3.f);
  t = aosq_eq(q0, q1);
  cast.f = vf4_x(t); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_y(t); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_z(t); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_w(t); CHECK(cast.i, (int)0xFFFFFFFF);

  q1 = vf4_set(0.f, 2.f, 3.f, -3.f);
  t = aosq_eq(q0, q1);
  cast.f = vf4_x(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_w(t); CHECK(cast.i, (int)0x00000000);

  q1 = vf4_set(1.f, 0.f, 3.f, -3.f);
  t = aosq_eq(q0, q1);
  cast.f = vf4_x(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_w(t); CHECK(cast.i, (int)0x00000000);

  q1 = vf4_set(1.f, 2.f, 0.f, -3.f);
  t = aosq_eq(q0, q1);
  cast.f = vf4_x(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_w(t); CHECK(cast.i, (int)0x00000000);

  q1 = vf4_set(1.f, 2.f, 3.f, 0.f);
  t = aosq_eq(q0, q1);
  cast.f = vf4_x(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_w(t); CHECK(cast.i, (int)0x00000000);

  q1 = vf4_set(1.01f, 2.f, 3.02f, -3.f);
  t = aosq_eq_eps(q0, q1, vf4_set1(0.01f));
  cast.f = vf4_x(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_y(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_z(t); CHECK(cast.i, (int)0x00000000);
  cast.f = vf4_w(t); CHECK(cast.i, (int)0x00000000);
  t = aosq_eq_eps(q0, q1, vf4_set1(0.02f));
  cast.f = vf4_x(t); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_y(t); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_z(t); CHECK(cast.i, (int)0xFFFFFFFF);
  cast.f = vf4_w(t); CHECK(cast.i, (int)0xFFFFFFFF);

  q0 = vf4_set(1.f, 2.f, 3.f, 4.f);
  q1 = vf4_set(5.f, 6.f, 7.f, 8.f);
  q2 = aosq_mul(q0, q1);
  CHECK(vf4_x(q2), 24.f);
  CHECK(vf4_y(q2), 48.f);
  CHECK(vf4_z(q2), 48.f);
  CHECK(vf4_w(q2), -6.f);

  q2 = aosq_conj(q0);
  CHECK(vf4_x(q2), -1.f);
  CHECK(vf4_y(q2), -2.f);
  CHECK(vf4_z(q2), -3.f);
  CHECK(vf4_w(q2), 4.f);

  q0 = vf4_normalize(vf4_set(1.f, 2.f, 5.f, 0.5f));
  q1 = vf4_xyzz(q0);
  q1 = vf4_xyzd(q1, aosq_calca(q1));
  CHECK(vf4_x(q0), vf4_x(q1));
  CHECK(vf4_y(q0), vf4_y(q1));
  CHECK(vf4_z(q0), vf4_z(q1));
  CHECK(EQ_EPS(vf4_w(q0), vf4_w(q1), 1.e-6f), true);

  q0 = vf4_set(1.f, 2.f, 3.f, 5.f);
  q1 = vf4_set(2.f, 6.f, 7.f, 6.f);
  q2 = aosq_slerp(q0, q1, vf4_set1(0.3f));
  CHECK(EQ_EPS(vf4_x(q2), 1.3f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_y(q2), 3.2f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_z(q2), 4.2f, 1.e-6f), true);
  CHECK(EQ_EPS(vf4_w(q2), 5.3f, 1.e-6f), true);

  q0 = vf4_set(2.f, 5.f, 17.f, 9.f);
  aosq_to_aosf33(q0, &m);
  AOSF33_EQ_EPS
    (m,
     -627.f, 326.f, -22.f,
     -286.f, -585.f, 206.f,
     158.f, 134.f, -57.f,
     1.e-6f);

  q0 = vf4_normalize(q0);
  aosq_to_aosf33(q0, &m);
  AOSF33_EQ_EPS
    (m,
     -0.573935f, 0.817043f, -0.055138f,
     -0.716792f, -0.468672f, 0.516291f,
     0.395990f, 0.335840f, 0.854637f,
     1.e-6f);
}

#include <stdio.h>

int
main(int argc UNUSED, char** argv UNUSED)
{
  test_vf4();
  test_vi4();
  test_aosf33();
  test_aosf44();
  test_aosq();
  return 0;
}

