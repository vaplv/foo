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
  const int64_t nsec_per_usec = 1000;
  const int64_t nsec_per_msec = 1000 * nsec_per_usec;
  const int64_t nsec_per_sec = (1000 * nsec_per_msec);
  const int64_t nsec_per_min = (60 * nsec_per_sec);
  const int64_t nsec_per_hour = (60 * nsec_per_min);
  const int64_t nsec_per_day = (24 * nsec_per_hour);
  size_t available_dump_space = max_dump_len ? max_dump_len - 1 : 0;
  int64_t time_nsec = 0;

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

  time_nsec = TIME_TO_NSEC__(time);
  if(flag & TIME_DAY) {
    const int64_t nb_days = time_nsec / nsec_per_day;
    DUMP(nb_days, "day");
    time_nsec -= nb_days * nsec_per_day;
  }
  if(flag & TIME_HOUR) {
    const int64_t nb_hours = time_nsec / nsec_per_hour;
    DUMP(nb_hours, "hour");
    time_nsec -= nb_hours * nsec_per_hour;
  }
  if(flag & TIME_MIN) {
    const int64_t nb_mins = time_nsec / nsec_per_min;
    DUMP(nb_mins, "min");
    time_nsec -= nb_mins * nsec_per_min;
  }
  if(flag & TIME_SEC) {
    const int64_t nb_secs = time_nsec / nsec_per_sec;
    DUMP(nb_secs, "sec");
    time_nsec -= nb_secs * nsec_per_sec;
  }
  if(flag & TIME_MSEC) {
    const int64_t nb_msecs = time_nsec / nsec_per_msec;
    DUMP(nb_msecs, "msec");
    time_nsec -= nb_msecs * nsec_per_msec;
  }
  if(flag & TIME_USEC) {
    const int64_t nb_usecs = time_nsec / nsec_per_usec;
    DUMP(nb_usecs, "usec");
    time_nsec -= nb_usecs * nsec_per_usec;
  }
  if(flag & TIME_NSEC)
    DUMP(time_nsec, "nsec");

  #undef DUMP

  if(dump) {
    /* Remove last space. */
    const size_t last_char = strlen(dump) - 1;
    assert(dump[last_char] == ' ');
    dump[last_char] = '\0';
  }
}

