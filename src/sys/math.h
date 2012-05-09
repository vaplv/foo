#ifndef SYS_MATH_H
#define SYS_MATH_H

#define PI 3.14159265358979323846

#define DEG2RAD(x) \
  ((x)*0.0174532925199432957692369076848861L)

#define RAD2DEG(x) \
  ((x)*57.2957795130823208767981548141051703L)

#define IS_POWER_OF_2(i) \
  ((i) > 0 && ((i) & ((i)-1)) == 0)

#define MAX(a, b) \
  ((a) > (b) ? (a) : (b))

#define MIN(a, b) \
  ((a) < (b) ? (a) : (b))

#endif /* SYS_MATH_H */

