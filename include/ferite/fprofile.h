#ifndef __FERITE_PROFILE_H__
#define __FERITE_PROFILE_H__

#include "time.h"

#define FERITE_PROFILE_BEGIN(depth, filename, line) if (ferite_profile_enabled) ferite_profile_begin(filename, line, depth)
#define FERITE_PROFILE_END(depth, filename, line) if (ferite_profile_enabled) ferite_profile_end(filename, line, depth)
#define FERITE_PROFILE_SAVE() if (ferite_profile_enabled) ferite_profile_save()

extern int ferite_profile_enabled;

struct profile_entry {
	char *filename;
	int line;
	unsigned int ncalls;
	struct timespec total_duration;

	FeriteStack *stack;

	struct profile_entry *next; // next in hash table
};

void ferite_profile_save();
void ferite_profile_begin(char *filename, size_t line, unsigned int depth);
void ferite_profile_end(char *filename, size_t line, unsigned int depth);
// FIXME rename this to ferite_profile_set_filename_pattern
void ferite_profile_output(char *filename);
#endif
