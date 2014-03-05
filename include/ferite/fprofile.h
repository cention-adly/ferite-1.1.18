#ifndef __FERITE_PROFILE_H__
#define __FERITE_PROFILE_H__

#include "time.h"

extern int ferite_profile_enabled;
#define FERITE_PROFILE_BEGIN(filename, line) \
	do { if (ferite_profile_enabled) { \
		if (strncmp(filename, "eval()", 6) != 0) \
			ferite_profile_begin(filename, line); \
	}} while(0)
#define FERITE_PROFILE_END(filename, line) \
	do { if (ferite_profile_enabled) { \
		if (strncmp(filename, "eval()", 6) != 0) \
			ferite_profile_end(filename, line); \
	}} while(0)

void ferite_profile_begin(char *filename, size_t line);
void ferite_profile_end(char *filename, size_t line);
void ferite_profile_save(pid_t pid);
// FIXME rename this to ferite_profile_set_filename_pattern
void ferite_profile_output(char *filename);
#endif
