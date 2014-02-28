#ifndef __FERITE_PROFILE_H__
#define __FERITE_PROFILE_H__

#include "time.h"

#define FERITE_PROFILE_BEGIN(profile, filename, line) \
	do { if (ferite_profile_enabled) \
		ferite_profile_begin(profile, filename, line); \
	} while(0)
#define FERITE_PROFILE_END(profile, filename, line) \
	do { if (ferite_profile_enabled) \
		ferite_profile_end(profile, filename, line); \
	} while(0)

void ferite_profile_save(FeriteProfile *profile);
void ferite_profile_begin(FeriteProfile *profile, char *filename, size_t line);
void ferite_profile_end(FeriteProfile *profile, char *filename, size_t line);
// FIXME rename this to ferite_profile_set_filename_pattern
void ferite_profile_output(char *filename);
#endif
