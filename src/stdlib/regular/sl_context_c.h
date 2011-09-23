#ifndef SL_CONTEXT_C_H
#define SL_CONTEXT_C_H

#define SL_IS_POWER_OF_2(i) ((i) > 0 && ((i) & ((i)-1)) == 0)

#define SL_NEXT_POWER_OF_2(i, j) \
  (j) =  (i > 0) ? (i) - 1 : (i), \
  (j) |= (j) >> 1, \
  (j) |= (j) >> 2, \
  (j) |= (j) >> 4, \
  (j) |= (j) >> 8, \
  (sizeof(i) > 2 ? (j) |= (j) >> 16, (void)0 : (void)0), \
  (sizeof(i) > 8 ? (j) |= (j) >> 32, (void)0 : (void)0), \
  ++(j)

struct sl_context {
  /* Put the global structures/constants here. */
  char dummy;
};

#endif /* SL_CONTEXT_C_H */

