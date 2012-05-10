#include "sys/clock_time.h"
#include <inttypes.h>
#include <string.h>

#define NSEC_PER_USEC 1000L
#define NSEC_PER_MSEC (1000L * NSEC_PER_USEC)
#define NSEC_PER_SEC (1000L * NSEC_PER_MSEC)
#define NSEC_PER_MIN (60L * NSEC_PER_SEC)
#define NSEC_PER_HOUR (60L * NSEC_PER_MIN)
#define NSEC_PER_DAY (24L * NSEC_PER_HOUR)

EXPORT_SYM int64_t
time_val(const struct time* time, enum time_unit unit)
{
  int64_t val = TIME_TO_NSEC__(time);
  switch(unit) {
    case TIME_NSEC:
      /* Do nothing. */
      break;
    case TIME_USEC:
      val /= NSEC_PER_USEC;
      break;
    case TIME_MSEC:
      val /= NSEC_PER_MSEC;
      break;
    case TIME_SEC:
      val /= NSEC_PER_SEC;
      break;
    case TIME_MIN:
      val /= NSEC_PER_MIN;
      break;
    case TIME_HOUR:
      val /= NSEC_PER_HOUR;
      break;
    case TIME_DAY:
      val /= NSEC_PER_DAY;
      break;
    default: assert(0); break;
  }
  return val;
}

EXPORT_SYM void
time_dump
  (const struct time* time,
   int flag,
   size_t* real_dump_len,
   char* dump,
   size_t max_dump_len)
{
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
    const int64_t nb_days = time_nsec / NSEC_PER_DAY;
    DUMP(nb_days, "day");
    time_nsec -= nb_days * NSEC_PER_DAY;
  }
  if(flag & TIME_HOUR) {
    const int64_t nb_hours = time_nsec / NSEC_PER_HOUR;
    DUMP(nb_hours, "hour");
    time_nsec -= nb_hours * NSEC_PER_HOUR;
  }
  if(flag & TIME_MIN) {
    const int64_t nb_mins = time_nsec / NSEC_PER_MIN;
    DUMP(nb_mins, "min");
    time_nsec -= nb_mins * NSEC_PER_MIN;
  }
  if(flag & TIME_SEC) {
    const int64_t nb_secs = time_nsec / NSEC_PER_SEC;
    DUMP(nb_secs, "sec");
    time_nsec -= nb_secs * NSEC_PER_SEC;
  }
  if(flag & TIME_MSEC) {
    const int64_t nb_msecs = time_nsec / NSEC_PER_MSEC;
    DUMP(nb_msecs, "msec");
    time_nsec -= nb_msecs * NSEC_PER_MSEC;
  }
  if(flag & TIME_USEC) {
    const int64_t nb_usecs = time_nsec / NSEC_PER_USEC;
    DUMP(nb_usecs, "usec");
    time_nsec -= nb_usecs * NSEC_PER_USEC;
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

#undef NSEC_PER_USEC
#undef NSEC_PER_MSEC
#undef NSEC_PER_SEC
#undef NSEC_PER_MIN
#undef NSEC_PER_HOUR
#undef NSEC_PER_DAY

