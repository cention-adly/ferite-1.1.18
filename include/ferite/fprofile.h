#ifndef __FERITE_PROFILE_H__
#define __FERITE_PROFILE_H__ 1

#include "time.h"

struct ferite_profile_timings {
	struct ferite_profile_timings *next, *prev;
	struct timespec begin;
	struct timespec end;
};

struct ferite_profile_function_call {
	char *filename;
	int line;
	char *function_name;
	unsigned int ncalls;
	struct timespec total_duration;

	struct ferite_profile_timings *head;
	struct ferite_profile_timings *curr;
	struct ferite_profile_timings *tail;
	struct ferite_profile_timings *timings;

	struct ferite_profile_function_call *next; // next in hash table
};

void ferite_trace_init();
void ferite_trace_function_entry(int level, char *file, unsigned int line, char *name);
void ferite_trace_function_entry(int level, char *file, unsigned int line, char *name);
void ferite_trace_function_exit(int level, char *file, unsigned int line, char *name);
void ferite_trace_record();
#endif
