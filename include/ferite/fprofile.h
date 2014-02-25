#ifndef __FERITE_PROFILE_H__
#define __FERITE_PROFILE_H__

#include "time.h"

#define FERITE_PROFILE_BEGIN(script) if (ferite_profile_enabled) ferite_profile_begin(script)
#define FERITE_PROFILE_END(script) if (ferite_profile_enabled) ferite_profile_end(script)
#define FERITE_PROFILE_SAVE() if (ferite_profile_enabled) ferite_profile_save(script)

extern int ferite_profile_enabled;

struct profile_entry {
	char *filename;
	int line;
	unsigned int ncalls;
	struct timespec total_duration;

	FeriteStack *stack;

	struct profile_entry *next; // next in hash table
};

void ferite_trace_init();
void ferite_profile_output(char *filename);
#endif
