#ifndef __FERITE_PROFILE_H__
#define __FERITE_PROFILE_H__ 1

#include "time.h"

#define FERITE_PROFILE_BEGIN(script) ferite_profile_begin(script)
#define FERITE_PROFILE_END(script) ferite_profile_end(script)
#define FERITE_PROFILE_SAVE() ferite_profile_save(script)

struct profile_entry {
	char *filename;
	int line;
	unsigned int ncalls;
	struct timespec total_duration;

	FeriteStack *stack;

	struct profile_entry *next; // next in hash table
};

void ferite_trace_init();
void ferite_trace_function_entry(int level, char *file, unsigned int line, char *name);
void ferite_trace_function_entry(int level, char *file, unsigned int line, char *name);
void ferite_trace_function_exit(int level, char *file, unsigned int line, char *name);
#endif
