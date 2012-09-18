#include "maths/simd/aosf44.h"
#include "sys/sys.h"

vf4_t
aosf44_inverse(struct aosf44* res, const struct aosf44* m)
{
  /* Retrieve the columns 0, 1, 2 and 3 and the row 3 of the "m" matrix. */
  const vf4_t c0 = m->c0;
  const vf4_t c1 = m->c1;
  const vf4_t c2 = m->c2;
  const vf4_t c3 = m->c3;
  const vf4_t r3 = aosf44_row3(m);

  /* Define the 3x3 sub-matrix and compute their determinant */
  const struct aosf33 f33_012_012 = { c0, c1, c2 };
  const struct aosf33 f33_012_013 = { c0, c1, c3 };
  const struct aosf33 f33_012_023 = { c0, c2, c3 };
  const struct aosf33 f33_012_123 = { c1, c2, c3 };
  const vf4_t det_012 = vf4_048C
    (aosf33_det(&f33_012_123),
     aosf33_det(&f33_012_023),
     aosf33_det(&f33_012_013),
     aosf33_det(&f33_012_012));

  const vf4_t f33_023_c0 = vf4_xzww(c0);
  const vf4_t f33_023_c1 = vf4_xzww(c1);
  const vf4_t f33_023_c2 = vf4_xzww(c2);
  const vf4_t f33_023_c3 = vf4_xzww(c3);
  const struct aosf33 f33_023_012 = { f33_023_c0, f33_023_c1, f33_023_c2 };
  const struct aosf33 f33_023_013 = { f33_023_c0, f33_023_c1, f33_023_c3 };
  const struct aosf33 f33_023_023 = { f33_023_c0, f33_023_c2, f33_023_c3 };
  const struct aosf33 f33_023_123 = { f33_023_c1, f33_023_c2, f33_023_c3 };
  const vf4_t det_023 = vf4_048C
    (aosf33_det(&f33_023_123),
     aosf33_det(&f33_023_023),
     aosf33_det(&f33_023_013),
     aosf33_det(&f33_023_012));

  const vf4_t f33_123_c0 = vf4_yzww(c0);
  const vf4_t f33_123_c1 = vf4_yzww(c1);
  const vf4_t f33_123_c2 = vf4_yzww(c2);
  const vf4_t f33_123_c3 = vf4_yzww(c3);
  const struct aosf33 f33_123_012 = { f33_123_c0, f33_123_c1, f33_123_c2 };
  const struct aosf33 f33_123_013 = { f33_123_c0, f33_123_c1, f33_123_c3 };
  const struct aosf33 f33_123_023 = { f33_123_c0, f33_123_c2, f33_123_c3 };
  const struct aosf33 f33_123_123 = { f33_123_c1, f33_123_c2, f33_123_c3 };
  const vf4_t det_123 = vf4_048C
    (aosf33_det(&f33_123_123),
     aosf33_det(&f33_123_023),
     aosf33_det(&f33_123_013),
     aosf33_det(&f33_123_012));

  const vf4_t f33_013_c0 = vf4_xyww(c0);
  const vf4_t f33_013_c1 = vf4_xyww(c1);
  const vf4_t f33_013_c2 = vf4_xyww(c2);
  const vf4_t f33_013_c3 = vf4_xyww(c3);
  const struct aosf33 f33_013_012 = { f33_013_c0, f33_013_c1, f33_013_c2 };
  const struct aosf33 f33_013_013 = { f33_013_c0, f33_013_c1, f33_013_c3 };
  const struct aosf33 f33_013_023 = { f33_013_c0, f33_013_c2, f33_013_c3 };
  const struct aosf33 f33_013_123 = { f33_013_c1, f33_013_c2, f33_013_c3 };
  const vf4_t det_013 = vf4_048C
    (aosf33_det(&f33_013_123),
     aosf33_det(&f33_013_023),
     aosf33_det(&f33_013_013),
     aosf33_det(&f33_013_012));

  /* Compute the cofactors of the column 3 */
  const vf4_t cofacts = vf4_mul(det_012, vf4_set(-1.f, 1.f, -1.f, 1.f));

  /* Compute the determinant of the "m" matrix */
  const vf4_t det = vf4_dot(cofacts, r3);

  /* Invert the matrix */
  const vf4_t idet = vf4_rcp(det);
  const vf4_t mpmp_idet = vf4_mul(idet, vf4_set(-1.f, 1.f, -1.f, 1.f));
  const vf4_t pmpm_idet = vf4_mul(idet, vf4_set(1.f, -1.f, 1.f, -1.f));
  res->c0 = vf4_mul(det_123, pmpm_idet);
  res->c1 = vf4_mul(det_023, mpmp_idet);
  res->c2 = vf4_mul(det_013, pmpm_idet);
  res->c3 = vf4_mul(det_012, mpmp_idet);

  return det;
}

