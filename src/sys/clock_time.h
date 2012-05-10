#ifndef TIME_H
#define TIME_H

#ifndef __unix__
  #error "Unsupported platform."
#else
  /* The clock functons are available from the POSIX.1-2008 standard. */
  #define _POSIX_C_SOURCE 200112L
  #include <sys/time.h>
#endif 

#include "sys/sys.h"
#include <assert.h>
#include <unistd.h>
#include <stddef.h>

enum time_flag {
  TIME_USEC = 0x01,
  TIME_MSEC = 0x02,
  TIME_SEC = 0x04,
  TIME_MIN = 0x08,
  TIME_HOUR = 0x10,
  TIME_DAY = 0x20
};

struct time {
  struct timeval val;
};

static FINLINE void
current_time(struct time* time)
{
  UNUSED int err = 0;
  assert(time);
  err = gettimeofday(&time->val, NULL);
  assert(err == 0);

}

static FINLINE void
time_sub(struct time* res, const struct time* a, const struct time* b)
{
  assert(res && a && b);
  res->val.tv_sec = a->val.tv_sec - b->val.tv_sec;
  res->val.tv_usec = a->val.tv_usec - b->val.tv_usec;
  if(res->val.tv_usec < 0) {
    --res->val.tv_sec;
    res->val.tv_usec += 1000000;
  }
}

static FINLINE void
time_add(struct time* res, const struct time* a, const struct time* b)
{
  assert(res && a && b);
  res->val.tv_sec = a->val.tv_sec + b->val.tv_sec;
  res->val.tv_usec = a->val.tv_usec + b->val.tv_usec;
  if(res->val.tv_usec >= 1000000) {
	  ++res->val.tv_sec;
	  res->val.tv_usec -= 1000000;
  }
}

extern void
time_dump
  (const struct time* time,
   int flag,
   size_t* real_dump_len, /* May be NULL. */
   char* dump, /* May be NULL. */
   size_t max_dump_len);

#undef _POSIX_C_SOURCE

#endif /* TIME. */

