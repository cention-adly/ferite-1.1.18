#ifndef _TEST_TIME_H
#define _TEST_TIME_H
#include <time.h>

static inline unsigned long get_time_nanos(void)
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts))
		return 0;
	return (unsigned long) ts.tv_sec * 1000000000 + ts.tv_nsec;
}
#endif
