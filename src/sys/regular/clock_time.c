#include "sys/clock_time.h"
#include <inttypes.h>
#include <string.h>

EXPORT_SYM void
time_dump
  (const struct time* time,
   int flag,
   size_t* real_dump_len,
   char* dump,
   size_t max_dump_len)
{
  const int64_t usec_per_msec = 1000;
  const int64_t usec_per_sec = (1000 * usec_per_msec);
  const int64_t usec_per_min = (60 * usec_per_sec);
  const int64_t usec_per_hour = (60 * usec_per_min);
  const int64_t usec_per_day = (24 * usec_per_hour);
  size_t available_dump_space = max_dump_len ? max_dump_len - 1 : 0;
  int64_t time_usec = 0;

  assert(time && (!max_dump_len || dump));

  #define DUMP(time, suffix) \
    do { \
      const int len = snprintf \
        (dump, available_dump_space, \
         "%" PRIi64 " %s",time, time > 1 ? suffix "s ": suffix " "); \
      assert(len >= 0); \
      if(real_dump_len) { \
        real_dump_len += len; \
      } \
      if((size_t)len < available_dump_space) { \
        dump += len; \
        available_dump_space -= len; \
      } else if(dump) { \
        dump[available_dump_space] = '\0'; \
        available_dump_space = 0; \
        dump = NULL; \
      } \
    } while(0)

  time_usec = time->val.tv_usec + time->val.tv_sec * 1000000;
  if(flag & TIME_DAY) {
    const int64_t ndays = time_usec / usec_per_day;
    DUMP(ndays, "day");
    time_usec -= ndays * usec_per_day;
  }
  if(flag & TIME_HOUR) {
    const int64_t nhours = time_usec / usec_per_hour;
    DUMP(nhours, "hour");
    time_usec -= nhours * usec_per_hour;
  }
  if(flag & TIME_MIN) {
    const int64_t nmins = time_usec / usec_per_min;
    DUMP(nmins, "min");
    time_usec -= nmins * usec_per_min;
  }
  if(flag & TIME_SEC) {
    const int64_t nsecs = time_usec / usec_per_sec;
    DUMP(nsecs, "sec");
    time_usec -= nsecs * usec_per_sec;
  }
  if(flag & TIME_MSEC) {
    const int64_t nmsecs = time_usec / usec_per_msec;
    DUMP(nmsecs, "msec");
    time_usec -= nmsecs * usec_per_msec;
  }
  if(flag & TIME_USEC)
    DUMP(time_usec, "usec");

  #undef DUMP

  if(dump) {
    /* Remove last space. */
    const size_t last_char = strlen(dump) - 1;
    assert(dump[last_char] == ' ');
    dump[last_char] = '\0';
  }
}

