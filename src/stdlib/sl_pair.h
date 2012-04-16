#ifndef SL_PAIR
#define SL_PAIR

#define SL_IS_PAIR_VALID(pair) \
  (((pair)->key != NULL) & ((pair)->data != NULL))

struct sl_pair {
  void* key;
  void* data;
};

#endif /* SL_PAIR */
